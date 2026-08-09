#include "ModelDownloadDialog.h"
#include <QCloseEvent>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <windows.h>

static const QString HF_BASE =
    "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/";

ModelDownloadDialog::ModelDownloadDialog(const QString &modelsDir,
                                         QWidget *parent)
    : QDialog(parent), m_modelsDir(modelsDir) {
  setWindowTitle("Model Manager");
  setModal(true);
  setMinimumSize(820, 580);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

  setStyleSheet(
      "QDialog { background:#141414; color:#D0D0D0; }"
      "QTableWidget { background:#161616; border:1px solid #222; "
      "border-radius:6px; "
      "  gridline-color:#1E1E1E; color:#C8C8C8; font-size:8.5pt; outline:none; "
      "}"
      "QTableWidget::item { padding:6px 10px; border-bottom:1px solid #1E1E1E; "
      "}"
      "QTableWidget::item:selected { background:#212121; color:#E8872A; }"
      "QHeaderView::section { background:#1C1C1C; color:#505050; border:none; "
      "  border-bottom:1px solid #262626; padding:6px 10px; font-size:7.5pt; "
      "  letter-spacing:0.4px; font-weight:600; }"
      "QProgressBar { background:#1A1A1A; border:1px solid #262626; "
      "border-radius:4px; "
      "  min-height:10px; max-height:10px; text-align:center; }"
      "QProgressBar::chunk { background:#E8872A; border-radius:3px; }"
      "QLabel { color:#D0D0D0; font-size:8.5pt; }");

  buildCatalog();
  m_recommendedId = recommendedModelId();

  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(20, 18, 20, 18);
  vl->setSpacing(14);

  {
    auto *topRow = new QHBoxLayout();
    auto *titleLbl = new QLabel("Whisper Model Manager", this);
    titleLbl->setStyleSheet("color:#E0E0E0; font-size:11pt; font-weight:700;");
    topRow->addWidget(titleLbl);
    topRow->addStretch();

    m_systemInfoLabel = new QLabel(this);
    m_systemInfoLabel->setStyleSheet("color:#505050; font-size:8pt;");
    topRow->addWidget(m_systemInfoLabel);
    vl->addLayout(topRow);

    int ram = detectSystemRamMB();
    bool gpu = hasGpu();
    m_systemInfoLabel->setText(QString("System: %1 MB RAM  |  GPU: %2")
                                   .arg(ram)
                                   .arg(gpu ? "Detected" : "Not detected"));
  }

  {
    auto *recRow = new QHBoxLayout();
    auto *recIcon = new QLabel("(Rec)", this);
    recIcon->setStyleSheet("color:#E8872A; font-size:10pt;");
    recRow->addWidget(recIcon);

    m_recommendBadge = new QLabel(this);
    m_recommendBadge->setStyleSheet(
        "color:#E8872A; font-size:8.5pt; font-weight:600; "
        "background:#1F1A10; border:1px solid #3A2A10; "
        "border-radius:4px; padding:3px 10px;");
    recRow->addWidget(m_recommendBadge);
    recRow->addStretch();

    auto *refBtn = new QPushButton("Refresh", this);
    refBtn->setStyleSheet(
        "QPushButton { background:#1E1E1E; color:#606060; border:1px solid "
        "#2A2A2A; "
        "  border-radius:5px; padding:4px 12px; font-size:8pt; }"
        "QPushButton:hover { color:#A0A0A0; border-color:#383838; }");
    connect(refBtn, &QPushButton::clicked, [this] { populateTable(); });
    recRow->addWidget(refBtn);
    vl->addLayout(recRow);
  }

  m_table = new QTableWidget(0, 7, this);
  m_table->setHorizontalHeaderLabels({"Model", "Size", "RAM Req.", "Languages",
                                      "Speed", "Accuracy", "Status"});
  m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_table->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(
      4, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(
      5, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(
      6, QHeaderView::ResizeToContents);
  m_table->verticalHeader()->setVisible(false);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setShowGrid(false);
  m_table->setAlternatingRowColors(false);
  m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  connect(m_table, &QTableWidget::itemSelectionChanged, this,
          &ModelDownloadDialog::onSelectionChanged);
  vl->addWidget(m_table, 1);

  m_descLabel = new QLabel(
      "Select a model to see details. "
      "Recommended model is pre-selected based on your system memory.",
      this);
  m_descLabel->setWordWrap(true);
  m_descLabel->setStyleSheet("color:#505050; font-size:8pt; padding:2px 0;");
  vl->addWidget(m_descLabel);

  {
    auto *progRow = new QVBoxLayout();
    progRow->setSpacing(5);

    m_progressLabel = new QLabel("", this);
    m_progressLabel->setStyleSheet("color:#808080; font-size:8pt;");
    progRow->addWidget(m_progressLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    progRow->addWidget(m_progressBar);
    vl->addLayout(progRow);
  }

  {
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_downloadBtn = new QPushButton("Download Selected Model", this);
    m_downloadBtn->setEnabled(false);
    m_downloadBtn->setStyleSheet(
        "QPushButton { background:#E8872A; color:#0E0E0E; border:none; "
        "border-radius:6px; "
        "  padding:8px 20px; font-size:9pt; font-weight:700; min-height:32px; }"
        "QPushButton:hover { background:#F09840; }"
        "QPushButton:pressed { background:#C97020; }"
        "QPushButton:disabled { background:#2A2A2A; color:#404040; }");
    connect(m_downloadBtn, &QPushButton::clicked, this,
            &ModelDownloadDialog::onDownloadClicked);
    btnRow->addWidget(m_downloadBtn);

    m_cancelBtn = new QPushButton("Cancel Download", this);
    m_cancelBtn->setVisible(false);
    m_cancelBtn->setStyleSheet("QPushButton { background:#2A1A1A; "
                               "color:#E05050; border:1px solid #3A2020; "
                               "  border-radius:6px; padding:8px 18px; "
                               "font-size:9pt; min-height:32px; }"
                               "QPushButton:hover { background:#3A2020; }");
    connect(m_cancelBtn, &QPushButton::clicked, this,
            &ModelDownloadDialog::onCancelDownload);
    btnRow->addWidget(m_cancelBtn);

    btnRow->addStretch();

    m_closeBtn = new QPushButton("Close", this);
    m_closeBtn->setStyleSheet(
        "QPushButton { background:#222222; color:#808080; border:1px solid "
        "#2E2E2E; "
        "  border-radius:6px; padding:8px 18px; font-size:9pt; "
        "min-height:32px; }"
        "QPushButton:hover { color:#C0C0C0; border-color:#404040; }");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(m_closeBtn);

    vl->addLayout(btnRow);
  }

  populateTable();
}

void ModelDownloadDialog::buildCatalog() {
  m_catalog = {
      {"tiny", "Tiny", "ggml-tiny.bin",
       "Fastest, basic accuracy. Great for quick testing.", 75 * 1024 * 1024,
       512, "Fast", true, HF_BASE + "ggml-tiny.bin"},
      {"tiny.en", "Tiny (English)", "ggml-tiny.en.bin",
       "Fastest, English-only. Best choice for low-RAM machines.",
       75 * 1024 * 1024, 512, "Fast", false, HF_BASE + "ggml-tiny.en.bin"},
      {"base", "Base", "ggml-base.bin",
       "Good balance of speed and accuracy. Multilingual.", 142 * 1024 * 1024,
       1024, "Fast", true, HF_BASE + "ggml-base.bin"},
      {"base.en", "Base (English)", "ggml-base.en.bin",
       "Recommended for English. Fast with decent accuracy.", 142 * 1024 * 1024,
       1024, "Fast", false, HF_BASE + "ggml-base.en.bin"},
      {"small", "Small", "ggml-small.bin",
       "Good accuracy with moderate speed. Multilingual.", 466 * 1024 * 1024,
       2048, "Balanced", true, HF_BASE + "ggml-small.bin"},
      {"small.en", "Small (English)", "ggml-small.en.bin",
       "Recommended for English with good accuracy.", 466 * 1024 * 1024, 2048,
       "Balanced", false, HF_BASE + "ggml-small.en.bin"},
      {"medium", "Medium", "ggml-medium.bin",
       "High accuracy, multilingual. Needs 5+ GB RAM.", 1500 * 1024 * 1024,
       5120, "Accurate", true, HF_BASE + "ggml-medium.bin"},
      {"medium.en", "Medium (English)", "ggml-medium.en.bin",
       "High accuracy English-only. Faster than multilingual medium.",
       1500 * 1024 * 1024, 5120, "Accurate", false,
       HF_BASE + "ggml-medium.en.bin"},
      {"large-v2", "Large v2", "ggml-large-v2.bin",
       "Near-human accuracy. Multilingual. Requires 10+ GB RAM.",
       2900LL * 1024 * 1024, 10240, "Accurate", true,
       HF_BASE + "ggml-large-v2.bin"},
      {"large-v3", "Large v3", "ggml-large-v3.bin",
       "Best available accuracy. Latest version. Needs 10+ GB RAM.",
       2900LL * 1024 * 1024, 10240, "Accurate", true,
       HF_BASE + "ggml-large-v3.bin"},
  };
}

void ModelDownloadDialog::populateTable() {
  m_table->setRowCount(0);
  int recRow = -1;

  for (int i = 0; i < m_catalog.size(); ++i) {
    const ModelInfo &m = m_catalog[i];
    m_table->insertRow(i);

    auto *nameItem = new QTableWidgetItem(m.displayName);
    nameItem->setData(Qt::UserRole, i);
    if (m.id == m_recommendedId)
      nameItem->setForeground(QColor("#E8872A"));
    m_table->setItem(i, 0, nameItem);

    m_table->setItem(i, 1, new QTableWidgetItem(formatSize(m.sizeBytes)));
    m_table->setItem(
        i, 2, new QTableWidgetItem(QString("%1 MB").arg(m.ramRequiredMB)));
    m_table->setItem(
        i, 3,
        new QTableWidgetItem(m.multilingual ? "Multilingual" : "English only"));
    m_table->setItem(i, 4,
                     new QTableWidgetItem(m.tier == "Fast" ? "Fast"
                                          : m.tier == "Balanced"
                                              ? "Balanced"
                                              : "Accurate"));

    QString acc = (m.tier == "Fast")       ? "2/5"
                  : (m.tier == "Balanced") ? "3/5"
                                           : "5/5";
    m_table->setItem(i, 5, new QTableWidgetItem(acc));

    QString modelFile = m_modelsDir + "/" + m.filename;
    bool exists = QFileInfo::exists(modelFile);
    auto *statusItem =
        new QTableWidgetItem(exists ? "[Downloaded]" : "Not downloaded");
    statusItem->setForeground(exists ? QColor("#4CAF50") : QColor("#404040"));
    m_table->setItem(i, 6, statusItem);

    m_table->setRowHeight(i, 32);

    if (m.id == m_recommendedId)
      recRow = i;
  }

  if (recRow >= 0) {
    m_table->selectRow(recRow);
    m_table->scrollToItem(m_table->item(recRow, 0));

    const ModelInfo &rec = m_catalog[recRow];
    m_recommendBadge->setText(
        QString("Recommended for your system: %1  (%2 RAM required)")
            .arg(rec.displayName)
            .arg(formatSize(rec.ramRequiredMB * 1024LL * 1024LL)));
  }
}

void ModelDownloadDialog::onSelectionChanged() {
  QList<QTableWidgetItem *> sel = m_table->selectedItems();
  if (sel.isEmpty()) {
    m_downloadBtn->setEnabled(false);
    m_selectedRow = -1;
    return;
  }

  int row = m_table->row(sel.first());
  m_selectedRow = row;

  if (row < 0 || row >= m_catalog.size())
    return;
  const ModelInfo &m = m_catalog[row];

  bool exists = QFileInfo::exists(m_modelsDir + "/" + m.filename);
  m_downloadBtn->setText(exists ? "Re-download Model"
                                : "Download Selected Model");
  m_downloadBtn->setEnabled(!m_reply || !m_reply->isRunning());

  m_descLabel->setText(
      QString("<b>%1</b>  -  %2<br>"
              "<span style='color:#505050;'>Size: %3 &nbsp;|&nbsp; "
              "RAM: %4 MB minimum &nbsp;|&nbsp; %5</span>")
          .arg(m.displayName)
          .arg(m.description)
          .arg(formatSize(m.sizeBytes))
          .arg(m.ramRequiredMB)
          .arg(m.multilingual ? "Multilingual" : "English only"));
  m_descLabel->setTextFormat(Qt::RichText);
}

void ModelDownloadDialog::onDownloadClicked() {
  if (m_selectedRow < 0 || m_selectedRow >= m_catalog.size())
    return;
  m_downloading = m_catalog[m_selectedRow];

  QString outPath = m_modelsDir + "/" + m_downloading.filename;
  QDir().mkpath(m_modelsDir);

  m_outFile.setFileName(outPath + ".part");
  if (!m_outFile.open(QIODevice::WriteOnly)) {
    QMessageBox::critical(
        this, "Error",
        QString("Cannot write to models directory:\n%1").arg(m_modelsDir));
    return;
  }

  QNetworkRequest req(QUrl(m_downloading.downloadUrl));
  req.setRawHeader("User-Agent", "SubtitleGeneratorAI/2.0");
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                   QNetworkRequest::NoLessSafeRedirectPolicy);

  m_reply = m_net.get(req);
  connect(m_reply, &QNetworkReply::downloadProgress, this,
          &ModelDownloadDialog::onDownloadProgress);
  connect(m_reply, &QNetworkReply::finished, this,
          &ModelDownloadDialog::onDownloadFinished);
  connect(m_reply, &QNetworkReply::readyRead, this, [this] {
    if (m_reply && m_outFile.isOpen())
      m_outFile.write(m_reply->readAll());
  });

  setDownloading(true);
  m_progressLabel->setText(QString("Downloading %1 (%2)...")
                               .arg(m_downloading.displayName)
                               .arg(formatSize(m_downloading.sizeBytes)));
}

void ModelDownloadDialog::onDownloadProgress(qint64 received, qint64 total) {
  if (total > 0) {
    int pct = static_cast<int>(100.0 * received / total);
    m_progressBar->setValue(pct);
    m_progressLabel->setText(QString("Downloading %1  -  %2 / %3")
                                 .arg(m_downloading.displayName)
                                 .arg(formatSize(received))
                                 .arg(formatSize(total)));
  }
}

void ModelDownloadDialog::onDownloadFinished() {
  // Guard against re-entrancy: abort() emits finished() synchronously, and the
  // dialog may already have torn the reply down.
  if (!m_reply)
    return;
  QNetworkReply *reply = m_reply;
  m_reply = nullptr;

  m_outFile.flush();
  if (m_outFile.isOpen())
    m_outFile.close();

  if (reply->error() != QNetworkReply::NoError) {
    QFile::remove(m_outFile.fileName());
    m_progressLabel->setText(QString("Error: %1").arg(reply->errorString()));
    m_progressLabel->setStyleSheet("color:#E05050; font-size:8pt;");
  } else {
    QString finalPath = m_modelsDir + "/" + m_downloading.filename;
    QFile::remove(finalPath);
    QFile::rename(m_outFile.fileName(), finalPath);
    m_lastDownloaded = finalPath;

    m_progressLabel->setText(
        QString("[OK]  %1 downloaded successfully -> models/%2")
            .arg(m_downloading.displayName)
            .arg(m_downloading.filename));
    m_progressLabel->setStyleSheet("color:#4CAF50; font-size:8pt;");
    populateTable();
  }

  reply->deleteLater();
  setDownloading(false);
}

void ModelDownloadDialog::abortActiveDownload() {
  if (!m_reply)
    return;
  // Detach first so abort()'s synchronous finished() can't re-enter our slots
  // and double-free the reply.
  QNetworkReply *reply = m_reply;
  m_reply = nullptr;
  reply->disconnect();
  reply->abort();
  reply->deleteLater();

  if (m_outFile.isOpen())
    m_outFile.close();
  if (!m_outFile.fileName().isEmpty())
    QFile::remove(m_outFile.fileName());
}

void ModelDownloadDialog::onCancelDownload() {
  abortActiveDownload();
  setDownloading(false);
  m_progressLabel->setText("Download cancelled.");
  m_progressLabel->setStyleSheet("color:#808080; font-size:8pt;");
}

void ModelDownloadDialog::closeEvent(QCloseEvent *e) {
  // Closing the dialog (window X) mid-download would otherwise destroy the
  // reply on the network manager and fire finished() into a dead dialog.
  abortActiveDownload();
  QDialog::closeEvent(e);
}

void ModelDownloadDialog::setDownloading(bool active) {
  m_downloadBtn->setVisible(!active);
  m_cancelBtn->setVisible(active);
  m_progressBar->setVisible(active);
  m_closeBtn->setEnabled(!active);
  m_table->setEnabled(!active);
  if (!active)
    m_progressBar->setValue(0);
}

int ModelDownloadDialog::detectSystemRamMB() const {
  MEMORYSTATUSEX ms;
  ms.dwLength = sizeof(ms);
  if (GlobalMemoryStatusEx(&ms))
    return static_cast<int>(ms.ullTotalPhys / (1024 * 1024));
  return 4096;
}

bool ModelDownloadDialog::hasGpu() const { return false; }

QString ModelDownloadDialog::recommendedModelId() const {
  int ram = detectSystemRamMB();
  bool gpu = hasGpu();

  if (ram >= 10240 || gpu)
    return "large-v3";
  if (ram >= 5120)
    return "medium";
  if (ram >= 3000)
    return "small";
  if (ram >= 1500)
    return "base.en";
  return "tiny.en";
}

QString ModelDownloadDialog::formatSize(qint64 bytes) const {
  if (bytes >= 1024LL * 1024 * 1024)
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 1);
  if (bytes >= 1024 * 1024)
    return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 0);
  return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 0);
}

QString ModelDownloadDialog::lastDownloadedPath() const {
  return m_lastDownloaded;
}
QString ModelDownloadDialog::modelsDir() const { return m_modelsDir; }
