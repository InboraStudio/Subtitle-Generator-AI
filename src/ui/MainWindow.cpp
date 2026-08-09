#include "MainWindow.h"
#include "../logging/Logger.h"
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setAttribute(Qt::WA_TranslucentBackground, false);
  setAttribute(Qt::WA_DeleteOnClose);

  setWindowTitle("Subtitle Generator AI");
  setMinimumSize(960, 640);
  resize(1100, 720);

  m_logModel = new LogModel(this);
  m_queue = new JobQueue(this);
  m_processor = new BatchProcessor(m_queue, this);
  m_monitorWorker = new MonitorWorker(this);

  connect(Logger::instance(), &Logger::entryAdded, m_logModel,
          &LogModel::appendEntry, Qt::QueuedConnection);

  loadStylesheet();
  setupLayout();
  setupConnections();

  m_monitorWorker->start();

  QScreen *scr = QApplication::primaryScreen();
  if (scr) {
    QRect geo = scr->availableGeometry();
    move(geo.center() - rect().center());
  }

  LOG_INFO("Subtitle Generator AI v2 started");
}

MainWindow::~MainWindow() {
  m_monitorWorker->stopWorker();
  m_monitorWorker->wait(3000);
}

void MainWindow::loadStylesheet() {
  QFile f(":/styles/dark.qss");
  if (f.open(QFile::ReadOnly | QFile::Text))
    qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
}

void MainWindow::setupLayout() {
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  m_titleBar = new QWidget(this);
  m_titleBar->setObjectName("titleBar");
  m_titleBar->setFixedHeight(38);
  {
    auto *tl = new QHBoxLayout(m_titleBar);
    tl->setContentsMargins(14, 0, 4, 0);
    tl->setSpacing(4);

    auto *appLbl = new QLabel("Subtitle Generator AI", m_titleBar);
    appLbl->setObjectName("appTitle");
    tl->addWidget(appLbl);

    auto *verLbl = new QLabel("v2.0", m_titleBar);
    verLbl->setObjectName("appVersion");
    tl->addWidget(verLbl);
    tl->addStretch();

    auto *logBtn = new QPushButton("Console", m_titleBar);
    logBtn->setObjectName("secondaryBtn");
    logBtn->setCheckable(false);
    logBtn->setToolTip("Toggle the log console");
    logBtn->setStyleSheet(
        "QPushButton#secondaryBtn { min-height:24px; max-height:24px; "
        "min-width:72px; font-size:8.5pt; padding:2px 12px; }");
    connect(logBtn, &QPushButton::clicked, this, &MainWindow::toggleLogPanel);
    tl->addWidget(logBtn);
    tl->addSpacing(6);

    auto *minBtn = new QPushButton(m_titleBar);
    minBtn->setIcon(QIcon(":/icons/minimize.png"));
    minBtn->setObjectName("titleBtn");
    connect(minBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);
    tl->addWidget(minBtn);

    auto *maxBtn = new QPushButton(m_titleBar);
    maxBtn->setIcon(QIcon(":/icons/maximize.png"));
    maxBtn->setObjectName("titleBtn");
    connect(maxBtn, &QPushButton::clicked,
            [this] { isMaximized() ? showNormal() : showMaximized(); });
    tl->addWidget(maxBtn);

    auto *closeBtn = new QPushButton(m_titleBar);
    closeBtn->setIcon(QIcon(":/icons/close.png"));
    closeBtn->setObjectName("closeTitleBtn");
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);
    tl->addWidget(closeBtn);
  }
  root->addWidget(m_titleBar);

  auto *body = new QHBoxLayout();
  body->setContentsMargins(0, 0, 0, 0);
  body->setSpacing(0);

  m_sidebar = new SidebarWidget(this);
  body->addWidget(m_sidebar);

  auto *rightSide = new QFrame(this);
  rightSide->setObjectName("contentArea");
  auto *rsl = new QVBoxLayout(rightSide);
  rsl->setContentsMargins(0, 0, 0, 0);
  rsl->setSpacing(0);

  m_stack = new QStackedWidget(rightSide);
  m_inputPanel = new InputPanel(m_stack);
  m_settingsPanel = new SettingsPanel(m_stack);
  m_progressPanel = new ProgressPanel(m_stack);
  m_stack->addWidget(m_inputPanel);
  m_stack->addWidget(m_settingsPanel);
  m_stack->addWidget(m_progressPanel);
  rsl->addWidget(m_stack, 1);

  m_statsWidget = new SystemStatsWidget(rightSide);
  m_statsWidget->setFixedHeight(110);
  m_statsWidget->setStyleSheet(
      "background:#1A1A1A; border-top:1px solid #222222;");
  rsl->addWidget(m_statsWidget);

  m_logPanel = new LogPanel(m_logModel, rightSide);
  m_logPanel->setVisible(false);
  rsl->addWidget(m_logPanel);

  auto *statusBar = new QFrame(rightSide);
  statusBar->setObjectName("statusBar");
  statusBar->setFixedHeight(26);
  {
    auto *sl = new QHBoxLayout(statusBar);
    sl->setContentsMargins(12, 0, 12, 0);
    m_statusLabel = new QLabel("Ready", statusBar);
    m_statusLabel->setObjectName("statusText");
    sl->addWidget(m_statusLabel);
    sl->addStretch();
  }
  rsl->addWidget(statusBar);

  body->addWidget(rightSide, 1);
  root->addLayout(body, 1);
}

void MainWindow::setupConnections() {
  connect(m_sidebar, &SidebarWidget::tabSelected, this,
          &MainWindow::onTabSelected);
  connect(m_inputPanel, &InputPanel::startRequested, this,
          &MainWindow::onStartProcessing);

  connect(m_processor, &BatchProcessor::jobStarted, this,
          &MainWindow::onJobStarted);
  connect(m_processor, &BatchProcessor::jobProgress, this,
          &MainWindow::onJobProgress);
  connect(m_processor, &BatchProcessor::jobFinished, this,
          &MainWindow::onJobFinished);
  connect(m_processor, &BatchProcessor::jobFailed, this,
          &MainWindow::onJobFailed);
  connect(m_processor, &BatchProcessor::allJobsCompleted, this,
          &MainWindow::onAllComplete);

  connect(m_monitorWorker, &MonitorWorker::statsUpdated, m_statsWidget,
          &SystemStatsWidget::updateStats, Qt::QueuedConnection);
  connect(m_monitorWorker, &MonitorWorker::statsUpdated, this,
          &MainWindow::updateStatusBar, Qt::QueuedConnection);

  connect(m_processor, &BatchProcessor::jobStarted, m_progressPanel,
          &ProgressPanel::onJobStarted);
  connect(m_processor, &BatchProcessor::jobProgress, m_progressPanel,
          &ProgressPanel::onJobProgress);
  connect(m_processor, &BatchProcessor::jobFinished, m_progressPanel,
          &ProgressPanel::onJobFinished);
  connect(m_processor, &BatchProcessor::jobFailed, m_progressPanel,
          &ProgressPanel::onJobFailed);
  connect(m_processor, &BatchProcessor::allJobsCompleted, m_progressPanel,
          &ProgressPanel::onAllComplete);
}

void MainWindow::onTabSelected(int index) {
  m_stack->setCurrentIndex(index);
  m_sidebar->setActiveTab(index);
}

void MainWindow::onStartProcessing() {
  if (m_inputPanel->fileCount() == 0) {
    LOG_WARN("No files in queue");
    return;
  }

  m_progressPanel->reset();
  m_progressPanel->setTotalJobs(m_inputPanel->fileCount());

  QString model = m_settingsPanel->selectedModelPath();
  if (model.isEmpty()) {
    LOG_WARN("No AI model selected! Please download or select a whisper model "
             "in Settings first.");
    return;
  }

  QString outDir = m_settingsPanel->outputDirectory();
  QString srcLang = m_settingsPanel->sourceLanguage();
  QString tgtLang = m_settingsPanel->targetLanguage();
  bool translate = m_settingsPanel->translationEnabled();
  bool embed = m_settingsPanel->embedSubtitlesEnabled();
  bool fast = m_settingsPanel->fastModeEnabled();
  int parallel = m_settingsPanel->parallelJobCount();
  QString font = m_settingsPanel->subtitleFontName();
  int size = m_settingsPanel->subtitleFontSize();
  QString color = m_settingsPanel->subtitleFontColor();
  int outline = m_settingsPanel->subtitleOutlineThickness();
  int align = m_settingsPanel->subtitleAlignment();

  QList<Job> jobs =
      m_inputPanel->buildJobs(outDir, model, srcLang, tgtLang, translate, embed,
                              fast, font, size, color, outline, align);
  for (const Job &j : jobs)
    m_queue->enqueue(j);

  m_processor->start(parallel);
  m_sidebar->setActiveTab(2);
  m_stack->setCurrentIndex(2);

  m_statusLabel->setText(QString("Processing %1 file%2...")
                             .arg(jobs.size())
                             .arg(jobs.size() == 1 ? "" : "s"));
  LOG_INFO(QString("Started batch: %1 jobs, model=%2, parallel=%3")
               .arg(jobs.size())
               .arg(QFileInfo(model).fileName())
               .arg(parallel));
}

void MainWindow::onPauseProcessing() { m_processor->pause(); }
void MainWindow::onCancelProcessing() { m_processor->cancel(); }

void MainWindow::onJobStarted(int id, const QString &path) {
  LOG_INFO(
      QString("Job #%1 started: %2").arg(id).arg(QFileInfo(path).fileName()));
}

void MainWindow::onJobProgress(int id, int pct, const QString &stage) {
  Q_UNUSED(id)
  Q_UNUSED(pct)
  Q_UNUSED(stage)
}

void MainWindow::onJobFinished(int id, const QString &out) {
  LOG_INFO(QString("Job #%1 finished -> %2").arg(id).arg(out));
}

void MainWindow::onJobFailed(int id, const QString &err) {
  LOG_ERROR(QString("Job #%1 failed: %2").arg(id).arg(err));
}

void MainWindow::onAllComplete() {
  m_statusLabel->setText("All jobs completed");
  LOG_INFO("Batch processing complete");
}

void MainWindow::toggleLogPanel() {
  m_logVisible = !m_logVisible;
  m_logPanel->setVisible(m_logVisible);
}

void MainWindow::updateStatusBar(const SystemStats &stats) {
  m_statusLabel->setText(
      QString("CPU %1%  |  RAM %2 MB  |  GPU %3%")
          .arg(stats.cpuPercent, 0, 'f', 1)
          .arg(stats.ramUsedMB)
          .arg(stats.gpuAvailable ? QString::number(stats.gpuPercent, 'f', 1)
                                  : QString("N/A")));
}

void MainWindow::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton &&
      m_titleBar->geometry().contains(e->pos())) {
    m_dragging = true;
    m_dragStart = e->globalPosition().toPoint() - frameGeometry().topLeft();
  }
}

void MainWindow::mouseMoveEvent(QMouseEvent *e) {
  if (m_dragging && (e->buttons() & Qt::LeftButton))
    move(e->globalPosition().toPoint() - m_dragStart);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *) { m_dragging = false; }

void MainWindow::closeEvent(QCloseEvent *e) {
  m_processor->cancel();
  m_monitorWorker->stopWorker();
  e->accept();
}
