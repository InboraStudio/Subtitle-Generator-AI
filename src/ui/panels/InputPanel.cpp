#include "InputPanel.h"
#include "../../logging/Logger.h"
#include "../../pipeline/FileScanner.h"
#include "../Theme.h"
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QShortcut>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

InputPanel::InputPanel(QWidget *parent) : QWidget(parent) {
  setAcceptDrops(true);

  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(20, 18, 20, 18);
  vl->setSpacing(14);

  vl->addLayout(Theme::pageHeader(this, "Video Queue", &m_countLabel));

  auto *btnRow = new QHBoxLayout();
  btnRow->setSpacing(8);

  m_addFileBtn = new QPushButton("Add Files", this);
  m_addFileBtn->setObjectName("secondaryBtn");
  m_addFileBtn->setIcon(Theme::icon("file", Theme::Color::Text));
  m_addFileBtn->setToolTip("Add one or more video files (Ctrl+O)");
  connect(m_addFileBtn, &QPushButton::clicked, this, &InputPanel::addFile);
  btnRow->addWidget(m_addFileBtn);

  m_addFolderBtn = new QPushButton("Add Folder", this);
  m_addFolderBtn->setObjectName("secondaryBtn");
  m_addFolderBtn->setIcon(Theme::icon("folder", Theme::Color::Text));
  m_addFolderBtn->setToolTip(
      "Add every video in a folder and its subfolders (Ctrl+Shift+O)");
  connect(m_addFolderBtn, &QPushButton::clicked, this, &InputPanel::addFolder);
  btnRow->addWidget(m_addFolderBtn);

  btnRow->addStretch();

  m_removeBtn = new QPushButton("Remove", this);
  m_removeBtn->setObjectName("secondaryBtn");
  m_removeBtn->setToolTip("Remove the selected files (Delete)");
  connect(m_removeBtn, &QPushButton::clicked, this,
          &InputPanel::removeSelected);
  btnRow->addWidget(m_removeBtn);

  m_clearBtn = new QPushButton("Clear All", this);
  m_clearBtn->setObjectName("dangerBtn");
  m_clearBtn->setIcon(Theme::icon("clear", Theme::Color::Error));
  connect(m_clearBtn, &QPushButton::clicked, this, &InputPanel::clearAll);
  btnRow->addWidget(m_clearBtn);

  vl->addLayout(btnRow);

  m_stack = new QStackedWidget(this);

  // Empty state doubling as the drop target hint.
  m_dropZone = new QFrame(m_stack);
  m_dropZone->setObjectName("dropZone");
  {
    auto *dl = new QVBoxLayout(m_dropZone);
    dl->setAlignment(Qt::AlignCenter);
    dl->setSpacing(8);

    auto *ic = new QLabel(m_dropZone);
    ic->setPixmap(Theme::icon("input", Theme::Color::Accent).pixmap(40, 40));
    ic->setAlignment(Qt::AlignCenter);
    dl->addWidget(ic);
    dl->addSpacing(4);

    auto *t = new QLabel("Drop videos or folders here", m_dropZone);
    t->setObjectName("dropTitle");
    t->setAlignment(Qt::AlignCenter);
    dl->addWidget(t);

    auto *h = new QLabel("MP4, MKV, AVI, MOV, WEBM, FLV, WMV, M4V, TS, MPG  "
                         "-  folders are scanned recursively",
                         m_dropZone);
    h->setObjectName("dropHint");
    h->setAlignment(Qt::AlignCenter);
    h->setWordWrap(true);
    dl->addWidget(h);
    dl->addSpacing(10);

    auto *row = new QHBoxLayout();
    row->setSpacing(8);
    row->addStretch();
    auto *bFile = new QPushButton("Browse Files", m_dropZone);
    bFile->setObjectName("primaryBtn");
    connect(bFile, &QPushButton::clicked, this, &InputPanel::addFile);
    row->addWidget(bFile);
    auto *bFolder = new QPushButton("Browse Folder", m_dropZone);
    bFolder->setObjectName("secondaryBtn");
    connect(bFolder, &QPushButton::clicked, this, &InputPanel::addFolder);
    row->addWidget(bFolder);
    row->addStretch();
    dl->addLayout(row);
  }
  m_stack->addWidget(m_dropZone);

  m_table = new QTableWidget(0, 4, m_stack);
  m_table->setHorizontalHeaderLabels({"#", "File", "Size", "Location"});
  auto *hh = m_table->horizontalHeader();
  hh->setSectionResizeMode(0, QHeaderView::Fixed);
  hh->resizeSection(0, 44);
  hh->setSectionResizeMode(1, QHeaderView::Stretch);
  hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  hh->setSectionResizeMode(3, QHeaderView::Stretch);
  hh->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  hh->setHighlightSections(false);
  m_table->verticalHeader()->setVisible(false);
  m_table->verticalHeader()->setDefaultSectionSize(34);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setShowGrid(false);
  m_table->setWordWrap(false);
  m_table->setTextElideMode(Qt::ElideMiddle);
  m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_table->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_table, &QTableWidget::customContextMenuRequested, this,
          &InputPanel::showContextMenu);
  connect(m_table, &QTableWidget::itemSelectionChanged, this,
          &InputPanel::updateActions);
  connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row) {
    if (row >= 0 && row < m_filePaths.size())
      QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePaths[row]));
  });
  m_stack->addWidget(m_table);

  vl->addWidget(m_stack, 1);

  auto *footer = new QHBoxLayout();
  footer->setSpacing(10);
  m_summaryLabel = new QLabel(this);
  Theme::setRole(m_summaryLabel, "faint");
  footer->addWidget(m_summaryLabel, 1);

  m_startBtn = new QPushButton("Start Processing", this);
  m_startBtn->setObjectName("primaryBtn");
  m_startBtn->setIcon(Theme::icon("play", QColor("#0E0E0E")));
  m_startBtn->setMinimumHeight(36);
  m_startBtn->setMinimumWidth(180);
  m_startBtn->setToolTip("Generate subtitles for every file (Ctrl+Enter)");
  connect(m_startBtn, &QPushButton::clicked, this, &InputPanel::startRequested);
  footer->addWidget(m_startBtn);
  vl->addLayout(footer);

  auto *del = new QShortcut(QKeySequence::Delete, m_table);
  del->setContext(Qt::WidgetShortcut);
  connect(del, &QShortcut::activated, this, &InputPanel::removeSelected);

  refreshList();
}

void InputPanel::addFile() {
  if (m_busy)
    return;
  QStringList paths = QFileDialog::getOpenFileNames(
      this, "Add Video Files",
      QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
      "Video Files (*.mp4 *.mkv *.avi *.mov *.webm *.flv *.wmv *.m4v *.ts "
      "*.mpg *.mpeg);;All Files (*)");
  addFilePaths(paths);
}

void InputPanel::addFolder() {
  if (m_busy)
    return;
  QString folder = QFileDialog::getExistingDirectory(
      this, "Add Folder",
      QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
  if (folder.isEmpty())
    return;
  LOG_INFO(QString("Scanning folder: %1").arg(folder));
  addFilePaths(FileScanner::scan(folder, true));
}

void InputPanel::removeSelected() {
  if (m_busy)
    return;
  QList<int> rows;
  for (const QModelIndex &idx : m_table->selectionModel()->selectedRows())
    rows.append(idx.row());
  if (rows.isEmpty())
    return;
  std::sort(rows.begin(), rows.end(), std::greater<int>());
  for (int r : rows)
    if (r >= 0 && r < m_filePaths.size())
      m_filePaths.removeAt(r);
  refreshList();
  emit filesChanged(m_filePaths.size());
}

void InputPanel::clearAll() {
  if (m_busy || m_filePaths.isEmpty())
    return;
  m_filePaths.clear();
  refreshList();
  emit filesChanged(0);
}

void InputPanel::addFilePaths(const QStringList &paths) {
  int added = 0;
  for (const QString &p : paths) {
    const QString clean = QDir::cleanPath(p);
    if (!m_filePaths.contains(clean) && FileScanner::isValidVideoFile(clean)) {
      m_filePaths.append(clean);
      ++added;
    }
  }
  if (added == 0 && !paths.isEmpty())
    LOG_WARN("No new video files were added (duplicates or unsupported)");
  refreshList();
  if (added > 0) {
    m_table->scrollToBottom();
    emit filesChanged(m_filePaths.size());
  }
}

void InputPanel::refreshList() {
  m_table->setUpdatesEnabled(false);
  m_table->setRowCount(m_filePaths.size());
  qint64 total = 0;
  for (int i = 0; i < m_filePaths.size(); ++i) {
    QFileInfo fi(m_filePaths[i]);
    total += fi.size();

    auto *num = new QTableWidgetItem(QString::number(i + 1));
    num->setForeground(Theme::Color::TextFaint);
    m_table->setItem(i, 0, num);

    auto *name = new QTableWidgetItem(fi.fileName());
    name->setToolTip(m_filePaths[i]);
    name->setForeground(Theme::Color::TextStrong);
    m_table->setItem(i, 1, name);

    m_table->setItem(i, 2, new QTableWidgetItem(Theme::formatBytes(fi.size())));

    auto *loc = new QTableWidgetItem(QDir::toNativeSeparators(fi.absolutePath()));
    loc->setForeground(Theme::Color::TextMuted);
    loc->setToolTip(loc->text());
    m_table->setItem(i, 3, loc);
  }
  m_table->setUpdatesEnabled(true);

  const int n = m_filePaths.size();
  m_stack->setCurrentIndex(n == 0 ? 0 : 1);
  m_countLabel->setText(
      n == 0 ? "No files"
             : QString("%1 file%2  |  %3")
                   .arg(n)
                   .arg(n == 1 ? "" : "s")
                   .arg(Theme::formatBytes(total)));
  updateActions();
}

void InputPanel::updateActions() {
  const int n = m_filePaths.size();
  const bool hasSel = m_table->selectionModel()->hasSelection();
  m_addFileBtn->setEnabled(!m_busy);
  m_addFolderBtn->setEnabled(!m_busy);
  m_removeBtn->setEnabled(!m_busy && hasSel);
  m_clearBtn->setEnabled(!m_busy && n > 0);
  m_startBtn->setEnabled(!m_busy && n > 0);
  m_startBtn->setText(m_busy ? "Processing..." : "Start Processing");

  if (m_busy)
    m_summaryLabel->setText("A batch is running - see the Progress page.");
  else if (n == 0)
    m_summaryLabel->setText("Add videos to begin. Subtitles are generated "
                            "offline with Whisper.");
  else
    m_summaryLabel->setText(
        QString("%1 video%2 ready. Double-click a row to preview it, "
                "Delete to remove.")
            .arg(n)
            .arg(n == 1 ? "" : "s"));
}

void InputPanel::setBusy(bool busy) {
  m_busy = busy;
  setAcceptDrops(!busy);
  updateActions();
}

void InputPanel::showContextMenu(const QPoint &pos) {
  const int row = m_table->rowAt(pos.y());
  QMenu menu(this);
  QAction *open = menu.addAction("Open Video");
  QAction *reveal = menu.addAction("Open Containing Folder");
  menu.addSeparator();
  QAction *remove = menu.addAction("Remove Selected");
  QAction *clear = menu.addAction("Clear All");
  open->setEnabled(row >= 0);
  reveal->setEnabled(row >= 0);
  remove->setEnabled(!m_busy && m_table->selectionModel()->hasSelection());
  clear->setEnabled(!m_busy && !m_filePaths.isEmpty());

  QAction *chosen = menu.exec(m_table->viewport()->mapToGlobal(pos));
  if (!chosen)
    return;
  if (chosen == open)
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePaths[row]));
  else if (chosen == reveal)
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(QFileInfo(m_filePaths[row]).absolutePath()));
  else if (chosen == remove)
    removeSelected();
  else if (chosen == clear)
    clearAll();
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

void InputPanel::setDragActive(bool active) {
  m_dropZone->setProperty("dragActive", active ? "true" : "false");
  Theme::repolish(m_dropZone);
}

void InputPanel::dragEnterEvent(QDragEnterEvent *e) {
  if (!m_busy && e->mimeData()->hasUrls()) {
    e->acceptProposedAction();
    setDragActive(true);
  }
}

void InputPanel::dragLeaveEvent(QDragLeaveEvent *e) {
  setDragActive(false);
  QWidget::dragLeaveEvent(e);
}

void InputPanel::addPaths(const QStringList &paths) {
  if (m_busy)
    return;
  QStringList files;
  for (const QString &p : paths) {
    QFileInfo fi(p);
    if (fi.isDir())
      files += FileScanner::scan(fi.absoluteFilePath(), true);
    else if (FileScanner::isVideoFile(p))
      files.append(fi.absoluteFilePath());
  }
  addFilePaths(files);
}

void InputPanel::dropEvent(QDropEvent *e) {
  setDragActive(false);
  QStringList paths;
  for (const QUrl &url : e->mimeData()->urls())
    paths.append(url.toLocalFile());
  addPaths(paths);
  e->acceptProposedAction();
}
