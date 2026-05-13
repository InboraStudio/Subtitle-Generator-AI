#include "InputPanel.h"
#include "../../logging/Logger.h"
#include "../../pipeline/FileScanner.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

InputPanel::InputPanel(QWidget *parent) : QWidget(parent) {
  setAcceptDrops(true);

  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(16, 16, 16, 16);
  vl->setSpacing(12);

  auto *hdr = new QHBoxLayout();
  auto *title = new QLabel("Video Queue", this);
  title->setStyleSheet("color:#E0E0E0; font-size:10pt; font-weight:600;");
  hdr->addWidget(title);
  hdr->addStretch();

  m_countLabel = new QLabel("0 files", this);
  m_countLabel->setStyleSheet("color:#505050; font-size:8.5pt;");
  hdr->addWidget(m_countLabel);
  vl->addLayout(hdr);

  auto *btnRow = new QHBoxLayout();
  btnRow->setSpacing(6);

  m_addFileBtn = new QPushButton("+ Add File", this);
  m_addFileBtn->setObjectName("secondaryBtn");
  connect(m_addFileBtn, &QPushButton::clicked, this, &InputPanel::addFile);
  btnRow->addWidget(m_addFileBtn);

  m_addFolderBtn = new QPushButton("+ Add Folder", this);
  m_addFolderBtn->setObjectName("secondaryBtn");
  connect(m_addFolderBtn, &QPushButton::clicked, this, &InputPanel::addFolder);
  btnRow->addWidget(m_addFolderBtn);

  btnRow->addStretch();

  m_removeBtn = new QPushButton("Remove", this);
  m_removeBtn->setObjectName("secondaryBtn");
  connect(m_removeBtn, &QPushButton::clicked, this,
          &InputPanel::removeSelected);
  btnRow->addWidget(m_removeBtn);

  m_clearBtn = new QPushButton("Clear All", this);
  m_clearBtn->setObjectName("dangerBtn");
  connect(m_clearBtn, &QPushButton::clicked, this, &InputPanel::clearAll);
  btnRow->addWidget(m_clearBtn);

  vl->addLayout(btnRow);

  m_fileList = new QListWidget(this);
  m_fileList->setStyleSheet(
      "QListWidget { background:#161616; border:1px solid #222; "
      "border-radius:6px; }"
      "QListWidget::item { padding:5px 10px; border-bottom:1px solid #1E1E1E; "
      "color:#C0C0C0; font-size:8.5pt; }"
      "QListWidget::item:selected { background:#212121; color:#E0E0E0; }"
      "QListWidget::item:hover { background:#1E1E1E; }");
  m_fileList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  vl->addWidget(m_fileList, 1);

  auto *dropHint = new QLabel("Drag & drop video files or folders here", this);
  dropHint->setStyleSheet("color:#303030; font-size:8pt; padding:4px 0;");
  dropHint->setAlignment(Qt::AlignCenter);
  vl->addWidget(dropHint);

  m_startBtn = new QPushButton("Start Processing", this);
  m_startBtn->setObjectName("primaryBtn");
  m_startBtn->setMinimumHeight(36);
  m_startBtn->setStyleSheet(
      "QPushButton#primaryBtn { font-size:10pt; letter-spacing:0.3px; }");
  connect(m_startBtn, &QPushButton::clicked, this, &InputPanel::startRequested);
  vl->addWidget(m_startBtn);
}

void InputPanel::addFile() {
  QStringList paths = QFileDialog::getOpenFileNames(
      this, "Add Video Files",
      QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
      "Video Files (*.mp4 *.mkv *.avi *.mov *.webm *.flv *.wmv *.m4v *.ts "
      "*.mpg *.mpeg);;All Files (*)");
  addFilePaths(paths);
}

void InputPanel::addFolder() {
  QString folder = QFileDialog::getExistingDirectory(
      this, "Add Folder",
      QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
  if (folder.isEmpty())
    return;
  LOG_INFO(QString("Scanning folder: %1").arg(folder));
  addFilePaths(FileScanner::scan(folder, true));
}

void InputPanel::removeSelected() {
  QList<QListWidgetItem *> sel = m_fileList->selectedItems();
  for (auto *item : sel) {
    int row = m_fileList->row(item);
    m_filePaths.removeAt(row);
    delete m_fileList->takeItem(row);
  }
  m_countLabel->setText(QString("%1 file%2")
                            .arg(m_filePaths.size())
                            .arg(m_filePaths.size() == 1 ? "" : "s"));
  emit filesChanged(m_filePaths.size());
}

void InputPanel::clearAll() {
  m_filePaths.clear();
  m_fileList->clear();
  m_countLabel->setText("0 files");
  emit filesChanged(0);
}

void InputPanel::addFilePaths(const QStringList &paths) {
  for (const QString &p : paths) {
    if (!m_filePaths.contains(p) && FileScanner::isValidVideoFile(p)) {
      m_filePaths.append(p);
    }
  }
  refreshList();
  emit filesChanged(m_filePaths.size());
}

void InputPanel::refreshList() {
  m_fileList->clear();
  for (const QString &p : m_filePaths) {
    QFileInfo fi(p);
    auto *item = new QListWidgetItem(
        QString("%1  -  %2").arg(fi.fileName(), fi.absolutePath()), m_fileList);
    item->setToolTip(p);
    m_fileList->addItem(item);
  }
  m_countLabel->setText(QString("%1 file%2")
                            .arg(m_filePaths.size())
                            .arg(m_filePaths.size() == 1 ? "" : "s"));
}

int InputPanel::fileCount() const { return m_filePaths.size(); }

QList<Job> InputPanel::buildJobs(const QString &outputDir,
                                 const QString &modelPath,
                                 const QString &srcLang, const QString &tgtLang,
                                 bool translate, bool embedSubs, bool fastMode,
                                 const QString &fontName, int fontSize,
                                 const QString &fontColor, int outlineThickness,
                                 int alignment) const {
  QList<Job> jobs;
  for (const QString &path : m_filePaths) {
    Job job;
    job.inputPath = path;
    job.outputDirectory = outputDir;
    job.modelPath = modelPath;
    job.sourceLanguage = srcLang;
    job.targetLanguage = tgtLang;
    job.enableTranslation = translate;
    job.embedSubtitles = embedSubs;
    job.fastMode = fastMode;

    job.fontName = fontName;
    job.fontSize = fontSize;
    job.fontColorHex = fontColor;
    job.outlineThickness = outlineThickness;
    job.alignment = alignment;

    QFileInfo fi(path);
    QString base = outputDir + "/" + fi.completeBaseName();
    job.outputSrtPath = base + ".srt";
    job.outputVideoPath = embedSubs ? base + "_subbed.mp4" : QString();
    jobs.append(job);
  }
  return jobs;
}

void InputPanel::dragEnterEvent(QDragEnterEvent *e) {
  if (e->mimeData()->hasUrls())
    e->acceptProposedAction();
}

void InputPanel::dropEvent(QDropEvent *e) {
  QStringList paths;
  for (const QUrl &url : e->mimeData()->urls()) {
    QString local = url.toLocalFile();
    QFileInfo fi(local);
    if (fi.isDir())
      paths += FileScanner::scan(local, true);
    else if (FileScanner::isVideoFile(local))
      paths.append(local);
  }
  addFilePaths(paths);
  e->acceptProposedAction();
}
