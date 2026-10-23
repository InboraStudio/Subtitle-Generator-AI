#include "MainWindow.h"
#include "../logging/Logger.h"
#include "Theme.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QSizeGrip>
#include <QVBoxLayout>
#include <QWindow>

namespace {
const char *kPageNames[] = {"Input", "Settings", "Progress", "About"};
}

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
  setObjectName("mainWindow");
  setAttribute(Qt::WA_StyledBackground);
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setAttribute(Qt::WA_DeleteOnClose);

  setWindowTitle("Subtitle Generator AI");
  setMinimumSize(1000, 680);
  resize(1180, 780);

  m_logModel = new LogModel(this);
  m_queue = new JobQueue(this);
  m_processor = new BatchProcessor(m_queue, this);
  m_monitorWorker = new MonitorWorker(this);

  connect(Logger::instance(), &Logger::entryAdded, m_logModel,
          &LogModel::appendEntry, Qt::QueuedConnection);

  setupLayout();
  setupConnections();
  setupShortcuts();

  m_monitorWorker->start();

  QSettings s;
  const QByteArray geo = s.value("window/geometry").toByteArray();
  if (geo.isEmpty() || !restoreGeometry(geo)) {
    if (QScreen *scr = QApplication::primaryScreen()) {
      QRect avail = scr->availableGeometry();
      move(avail.center() - rect().center());
    }
  }
  if (s.value("window/console", false).toBool())
    toggleLogPanel();

  LOG_INFO("Subtitle Generator AI v2 started");
  QTimer::singleShot(600, this, &MainWindow::offerFirstModel);
}

void MainWindow::offerFirstModel() {
  // A fresh install has no Whisper model yet; point new users straight at
  // the one-click download instead of letting them discover it on Start.
  if (!m_settingsPanel->selectedModelPath().isEmpty())
    return;
  QMessageBox box(this);
  box.setWindowTitle("Welcome");
  box.setIcon(QMessageBox::Information);
  box.setText("Welcome to Subtitle Generator AI");
  box.setInformativeText(
      "One last step: download an AI model. The Model Manager picks the best "
      "one for this PC - it is a one-time download and then everything runs "
      "offline.");
  auto *get = box.addButton("Choose a Model", QMessageBox::AcceptRole);
  get->setObjectName("primaryBtn");
  Theme::repolish(get);
  box.addButton("Later", QMessageBox::RejectRole);
  box.exec();
  if (box.clickedButton() == get) {
    onTabSelected(1);
    m_settingsPanel->openModelManager();
  }
}

MainWindow::~MainWindow() {
  m_monitorWorker->stopWorker();
  m_monitorWorker->wait(3000);
}

void MainWindow::setupLayout() {
  auto *root = new QVBoxLayout(this);
  root->setContentsMargins(1, 1, 1, 1);
  root->setSpacing(0);

  // ---- Title bar -----------------------------------------------------------
  m_titleBar = new QFrame(this);
  m_titleBar->setObjectName("titleBar");
  m_titleBar->setFixedHeight(38);
  m_titleBar->installEventFilter(this);
  {
    auto *tl = new QHBoxLayout(m_titleBar);
    tl->setContentsMargins(12, 0, 0, 0);
    tl->setSpacing(8);

    auto *icon = new QLabel(m_titleBar);
    icon->setPixmap(QIcon(":/icons/app.png").pixmap(18, 18));
    tl->addWidget(icon);

    auto *appLbl = new QLabel("Subtitle Generator AI", m_titleBar);
    appLbl->setObjectName("appTitle");
    tl->addWidget(appLbl);

    m_pageLabel = new QLabel(m_titleBar);
    m_pageLabel->setObjectName("appVersion");
    tl->addWidget(m_pageLabel);
    tl->addStretch();

    m_consoleBtn = new QPushButton("Console", m_titleBar);
    m_consoleBtn->setObjectName("ghostBtn");
    m_consoleBtn->setCheckable(true);
    m_consoleBtn->setIcon(Theme::icon("log"));
    m_consoleBtn->setToolTip("Show or hide the log console (Ctrl+`)");
    connect(m_consoleBtn, &QPushButton::clicked, this,
            &MainWindow::toggleLogPanel);
    tl->addWidget(m_consoleBtn);
    tl->addSpacing(10);

    // Window controls sit flush against each other, like native captions.
    auto *controls = new QHBoxLayout();
    controls->setSpacing(0);
    tl->addLayout(controls);
    auto addTitleBtn = [&](const QString &name, const QString &glyph,
                           const QString &tip) {
      auto *b = new QPushButton(m_titleBar);
      b->setObjectName(name);
      b->setIcon(Theme::glyph(glyph, QColor("#A0A0A0")));
      b->setIconSize(QSize(10, 10));
      b->setToolTip(tip);
      b->setFocusPolicy(Qt::NoFocus);
      controls->addWidget(b);
      return b;
    };
    connect(addTitleBtn("titleBtn", "min", "Minimize"), &QPushButton::clicked,
            this, &MainWindow::showMinimized);
    m_maxBtn = addTitleBtn("titleBtn", "max", "Maximize (F11)");
    connect(m_maxBtn, &QPushButton::clicked, this,
            [this] { isMaximized() ? showNormal() : showMaximized(); });
    connect(addTitleBtn("closeTitleBtn", "close", "Close"),
            &QPushButton::clicked, this, &MainWindow::close);
  }
  root->addWidget(m_titleBar);

  // ---- Body ----------------------------------------------------------------
  auto *body = new QHBoxLayout();
  body->setContentsMargins(0, 0, 0, 0);
  body->setSpacing(0);

  m_sidebar = new SidebarWidget(this);
  body->addWidget(m_sidebar);

  auto *rightSide = new QWidget(this);
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
  m_aboutPanel = new AboutPanel(m_stack);
  m_stack->addWidget(m_aboutPanel);
  rsl->addWidget(m_stack, 1);

  m_statsWidget = new SystemStatsWidget(rightSide);
  rsl->addWidget(m_statsWidget);

  m_logPanel = new LogPanel(m_logModel, rightSide);
  m_logPanel->setVisible(false);
  rsl->addWidget(m_logPanel);

  auto *statusBar = new QFrame(rightSide);
  statusBar->setObjectName("statusBar");
  statusBar->setFixedHeight(26);
  {
    auto *sl = new QHBoxLayout(statusBar);
    sl->setContentsMargins(20, 0, 0, 0);
    sl->setSpacing(12);
    m_statusLabel = new QLabel("Ready", statusBar);
    m_statusLabel->setObjectName("statusText");
    sl->addWidget(m_statusLabel, 1);
    m_summaryLabel = new QLabel(statusBar);
    m_summaryLabel->setObjectName("statusText");
    sl->addWidget(m_summaryLabel);
    auto *grip = new QSizeGrip(statusBar);
    grip->setToolTip("Drag to resize");
    sl->addWidget(grip, 0, Qt::AlignBottom | Qt::AlignRight);
  }
  rsl->addWidget(statusBar);

  body->addWidget(rightSide, 1);
  root->addLayout(body, 1);

  // Transient notification, floated above the content.
  m_toast = new QLabel(this);
  m_toast->setObjectName("toast");
  m_toast->hide();
  m_toastTimer.setSingleShot(true);
  connect(&m_toastTimer, &QTimer::timeout, m_toast, &QLabel::hide);

  m_summaryLabel->setText(m_settingsPanel->summary());
  onTabSelected(0);
}

void MainWindow::setupConnections() {
  connect(m_sidebar, &SidebarWidget::tabSelected, this,
          &MainWindow::onTabSelected);
  connect(m_sidebar, &SidebarWidget::modelManagerRequested, m_settingsPanel,
          &SettingsPanel::openModelManager);
  connect(m_inputPanel, &InputPanel::startRequested, this,
          &MainWindow::onStartProcessing);
  connect(m_inputPanel, &InputPanel::filesChanged, this,
          &MainWindow::updateBadges);
  connect(m_settingsPanel, &SettingsPanel::settingsChanged, this, [this] {
    m_summaryLabel->setText(m_settingsPanel->summary());
  });
  connect(m_progressPanel, &ProgressPanel::pauseRequested, this,
          &MainWindow::onPauseProcessing);
  connect(m_progressPanel, &ProgressPanel::cancelRequested, this,
          &MainWindow::onCancelProcessing);
  connect(m_progressPanel, &ProgressPanel::addFilesRequested, this,
          [this] { onTabSelected(0); });
  connect(m_logPanel, &LogPanel::closeRequested, this,
          &MainWindow::toggleLogPanel);

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

  // Panel first, then ours, so badge counts read the panel's updated state.
  connect(m_processor, &BatchProcessor::jobStarted, this,
          &MainWindow::onJobStarted);
  connect(m_processor, &BatchProcessor::jobFinished, this,
          &MainWindow::onJobFinished);
  connect(m_processor, &BatchProcessor::jobFailed, this,
          &MainWindow::onJobFailed);
  connect(m_processor, &BatchProcessor::allJobsCompleted, this,
          &MainWindow::onAllComplete);

  connect(m_monitorWorker, &MonitorWorker::statsUpdated, m_statsWidget,
          &SystemStatsWidget::updateStats, Qt::QueuedConnection);
}

void MainWindow::setupShortcuts() {
  auto add = [this](const QKeySequence &seq, auto fn) {
    auto *sc = new QShortcut(seq, this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, fn);
  };
  for (int i = 0; i < 3; ++i)
    add(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), [this, i] { onTabSelected(i); });
  add(QKeySequence(Qt::CTRL | Qt::Key_O), [this] {
    onTabSelected(0);
    m_inputPanel->addFile();
  });
  add(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), [this] {
    onTabSelected(0);
    m_inputPanel->addFolder();
  });
  add(QKeySequence(Qt::CTRL | Qt::Key_Return), [this] { onStartProcessing(); });
  add(QKeySequence(Qt::CTRL | Qt::Key_Enter), [this] { onStartProcessing(); });
  add(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft), [this] { toggleLogPanel(); });
  add(QKeySequence(Qt::CTRL | Qt::Key_M),
      [this] { m_settingsPanel->openModelManager(); });
  add(QKeySequence(Qt::Key_F11),
      [this] { isMaximized() ? showNormal() : showMaximized(); });
}

void MainWindow::openPaths(const QStringList &paths) {
  if (paths.isEmpty())
    return;
  m_inputPanel->addPaths(paths);
  onTabSelected(0);
}

void MainWindow::onTabSelected(int index) {
  m_stack->setCurrentIndex(index);
  m_sidebar->setActiveTab(index);
  m_pageLabel->setText(QString("/  %1").arg(kPageNames[index]));
}

bool MainWindow::confirmCanStart() {
  if (m_inputPanel->fileCount() == 0) {
    showToast("Add at least one video first.", "error");
    onTabSelected(0);
    return false;
  }

  const QString model = m_settingsPanel->selectedModelPath();
  if (model.isEmpty() || !QFileInfo::exists(model)) {
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle("No AI model");
    box.setText("A Whisper model is needed to transcribe audio.");
    box.setInformativeText(
        "Open the Model Manager to download one. It recommends the best "
        "model for this PC.");
    auto *open = box.addButton("Open Model Manager", QMessageBox::AcceptRole);
    open->setObjectName("primaryBtn");
    Theme::repolish(open);
    box.addButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() == open) {
      onTabSelected(1);
      m_settingsPanel->openModelManager();
    }
    return false;
  }

  const QString outDir = m_settingsPanel->outputDirectory();
  if (outDir.isEmpty() || !QDir().mkpath(outDir)) {
    showToast("Choose a writable output folder in Settings.", "error");
    onTabSelected(1);
    return false;
  }

  if (m_settingsPanel->translationEnabled() &&
      m_settingsPanel->translationEndpoint().isEmpty()) {
    showToast("Translation is on but no server URL is set.", "error");
    onTabSelected(1);
    return false;
  }
  return true;
}

void MainWindow::onStartProcessing() {
  if (m_busy || !confirmCanStart())
    return;

  const QString model = m_settingsPanel->selectedModelPath();
  const QString outDir = m_settingsPanel->outputDirectory();
  const int parallel = m_settingsPanel->parallelJobCount();

  QList<Job> jobs = m_inputPanel->buildJobs(
      outDir, model, m_settingsPanel->sourceLanguage(),
      m_settingsPanel->targetLanguage(), m_settingsPanel->translationEnabled(),
      m_settingsPanel->embedSubtitlesEnabled(),
      m_settingsPanel->fastModeEnabled(), m_settingsPanel->subtitleFontName(),
      m_settingsPanel->subtitleFontSize(), m_settingsPanel->subtitleFontColor(),
      m_settingsPanel->subtitleOutlineThickness(),
      m_settingsPanel->subtitleAlignment());
  for (Job &j : jobs)
    j.translationEndpoint = m_settingsPanel->translationEndpoint();

  m_queue->reset();
  m_progressPanel->reset();
  m_progressPanel->setOutputDirectory(outDir);
  m_progressPanel->setTotalJobs(jobs.size());
  m_progressPanel->setJobs(jobs);
  for (const Job &j : jobs)
    m_queue->enqueue(j);

  m_cancelling = false;
  m_batchErrors = 0;
  setBusy(true);
  m_processor->start(parallel);
  onTabSelected(2);

  setStatus(QString("Processing %1 file%2...")
                .arg(jobs.size())
                .arg(jobs.size() == 1 ? "" : "s"),
            "accent");
  LOG_INFO(QString("Started batch: %1 jobs, model=%2, parallel=%3")
               .arg(jobs.size())
               .arg(QFileInfo(model).fileName())
               .arg(parallel));
}

void MainWindow::onPauseProcessing(bool paused) {
  if (paused) {
    m_processor->pause();
    setStatus("Paused", "accent");
    LOG_INFO("Queue paused");
  } else {
    m_processor->resume();
    setStatus("Processing...", "accent");
    LOG_INFO("Queue resumed");
  }
  updateBadges();
}

void MainWindow::onCancelProcessing() {
  if (!m_busy)
    return;
  if (QMessageBox::question(this, "Cancel Batch",
                            "Skip every file that has not started yet?\n\n"
                            "Files already running will finish first.") !=
      QMessageBox::Yes)
    return;
  m_cancelling = true;
  m_processor->cancel();
  m_progressPanel->onCancelled();
  setStatus("Cancelling...", "error");
  LOG_WARN("Batch cancelled by user");
  finishIfIdle();
}

void MainWindow::onJobStarted(int id, const QString &path) {
  LOG_INFO(
      QString("Job #%1 started: %2").arg(id).arg(QFileInfo(path).fileName()));
  updateBadges();
}

void MainWindow::onJobFinished(int id, const QString &out) {
  LOG_INFO(QString("Job #%1 finished -> %2").arg(id).arg(out));
  updateBadges();
  finishIfIdle();
}

void MainWindow::onJobFailed(int id, const QString &err) {
  LOG_ERROR(QString("Job #%1 failed: %2").arg(id).arg(err));
  ++m_batchErrors;
  updateBadges();
  finishIfIdle();
}

void MainWindow::finishIfIdle() {
  // After a cancel the processor stops dispatching and never emits
  // allJobsCompleted, so the batch ends when the last running file returns.
  if (!m_cancelling || m_processor->activeJobCount() > 0)
    return;
  m_cancelling = false;
  m_progressPanel->onCancelled();
  setBusy(false);
  setStatus("Batch cancelled", "error");
  showToast("Batch cancelled.", "error");
}

void MainWindow::onAllComplete() {
  setBusy(false);
  if (m_batchErrors == 0) {
    setStatus("All jobs completed", "success");
    showToast("All subtitles generated successfully.", "success");
  } else {
    setStatus(QString("Completed with %1 failure%2")
                  .arg(m_batchErrors)
                  .arg(m_batchErrors == 1 ? "" : "s"),
              "error");
    showToast(QString("Batch finished - %1 file%2 failed. See the Progress "
                      "page for details.")
                  .arg(m_batchErrors)
                  .arg(m_batchErrors == 1 ? "" : "s"),
              "error");
  }
  QApplication::alert(this);
  LOG_INFO("Batch processing complete");
}

void MainWindow::setBusy(bool busy) {
  m_busy = busy;
  m_inputPanel->setBusy(busy);
  updateBadges();
}

void MainWindow::updateBadges() {
  const int files = m_inputPanel->fileCount();
  m_sidebar->setBadge(0, files > 0 ? QString::number(files) : QString());
  if (m_busy) {
    const int active = m_progressPanel->activeJobs();
    m_sidebar->setBadge(2, active > 0 ? QString::number(active) : "...", true);
  } else {
    m_sidebar->setBadge(2, QString());
  }
}

void MainWindow::setStatus(const QString &text, const QString &role) {
  m_statusLabel->setText(text);
  Theme::setRole(m_statusLabel, role);
}

void MainWindow::showToast(const QString &text, const QString &role) {
  m_toast->setText(text);
  Theme::setRole(m_toast, role);
  m_toast->adjustSize();
  m_toast->raise();
  m_toast->show();
  resizeEvent(nullptr);
  m_toastTimer.start(4500);
}

void MainWindow::toggleLogPanel() {
  const bool show = !m_logPanel->isVisible();
  m_logPanel->setVisible(show);
  m_consoleBtn->setChecked(show);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e) {
  if (obj == m_titleBar) {
    if (e->type() == QEvent::MouseButtonPress) {
      auto *me = static_cast<QMouseEvent *>(e);
      // The system move gives native snapping (drag to the screen edge).
      if (me->button() == Qt::LeftButton && windowHandle()) {
        windowHandle()->startSystemMove();
        return true;
      }
    } else if (e->type() == QEvent::MouseButtonDblClick) {
      isMaximized() ? showNormal() : showMaximized();
      return true;
    }
  }
  return QWidget::eventFilter(obj, e);
}

void MainWindow::changeEvent(QEvent *e) {
  if (e->type() == QEvent::WindowStateChange) {
    // The 1px window outline only makes sense for a floating window.
    const bool max = isMaximized() || isFullScreen();
    const int m = max ? 0 : 1;
    layout()->setContentsMargins(m, m, m, m);
    m_maxBtn->setIcon(
        Theme::glyph(max ? "restore" : "max", QColor("#A0A0A0")));
    m_maxBtn->setToolTip(max ? "Restore (F11)" : "Maximize (F11)");
  }
  QWidget::changeEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *e) {
  if (e)
    QWidget::resizeEvent(e);
  if (m_toast && m_toast->isVisible()) {
    m_toast->setMaximumWidth(width() / 2);
    m_toast->adjustSize();
    const int bottom = height() - 26 - m_statsWidget->height() -
                       (m_logPanel->isVisible() ? m_logPanel->height() : 0);
    m_toast->move(width() - m_toast->width() - 20,
                  bottom - m_toast->height() - 16);
  }
}

void MainWindow::closeEvent(QCloseEvent *e) {
  if (m_busy &&
      QMessageBox::question(this, "Quit",
                            "Subtitles are still being generated. Quit "
                            "anyway?") != QMessageBox::Yes) {
    e->ignore();
    return;
  }
  QSettings s;
  s.setValue("window/geometry", saveGeometry());
  s.setValue("window/console", m_logPanel->isVisible());
  m_settingsPanel->saveSettings();

  m_processor->cancel();
  m_monitorWorker->stopWorker();
  e->accept();
}
