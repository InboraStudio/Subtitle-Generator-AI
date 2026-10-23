#include "ProgressPanel.h"
#include "../Theme.h"
#include <QDesktopServices>
#include <QDir>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QVBoxLayout>

ProgressPanel::ProgressPanel(QWidget *parent) : QWidget(parent) {
  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(20, 18, 20, 18);
  vl->setSpacing(14);

  {
    auto *hdr = Theme::pageHeader(this, "Processing");

    m_openBtn = new QPushButton("Open Output", this);
    m_openBtn->setObjectName("secondaryBtn");
    m_openBtn->setIcon(Theme::icon("folder", Theme::Color::Text));
    m_openBtn->setToolTip("Open the folder where subtitles are written");
    connect(m_openBtn, &QPushButton::clicked, this, [this] {
      if (m_outputDir.isEmpty())
        return;
      QDir().mkpath(m_outputDir);
      QDesktopServices::openUrl(QUrl::fromLocalFile(m_outputDir));
    });
    hdr->addWidget(m_openBtn);

    m_pauseBtn = new QPushButton("Pause", this);
    m_pauseBtn->setObjectName("secondaryBtn");
    m_pauseBtn->setIcon(Theme::icon("pause", Theme::Color::Text));
    m_pauseBtn->setToolTip("Hold the queue after the files already running");
    connect(m_pauseBtn, &QPushButton::clicked, this, [this] {
      m_paused = !m_paused;
      m_pauseBtn->setText(m_paused ? "Resume" : "Pause");
      m_pauseBtn->setIcon(
          Theme::icon(m_paused ? "play" : "pause", Theme::Color::Text));
      if (m_paused)
        setHeadline("Paused - running files will finish, then the queue waits",
                    "warning");
      else
        setHeadline("Processing...", "accent");
      emit pauseRequested(m_paused);
    });
    hdr->addWidget(m_pauseBtn);

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setObjectName("dangerBtn");
    m_cancelBtn->setIcon(Theme::icon("stop", Theme::Color::Error));
    m_cancelBtn->setToolTip("Skip every file that has not started yet");
    connect(m_cancelBtn, &QPushButton::clicked, this,
            &ProgressPanel::cancelRequested);
    hdr->addWidget(m_cancelBtn);
    vl->addLayout(hdr);
  }

  // ---- Overview card -------------------------------------------------------
  {
    QVBoxLayout *cl = nullptr;
    auto *card = Theme::card(this, "OVERVIEW", &cl);
    cl->setContentsMargins(18, 16, 18, 16);

    auto *row = new QHBoxLayout();
    row->setSpacing(24);

    m_overallCircle = new CircularProgressWidget(card);
    m_overallCircle->setThickness(8);
    m_overallCircle->setFixedSize(96, 96);
    row->addWidget(m_overallCircle);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(4);
    auto makeStat = [&](int col, const QString &label, QLabel *&value) {
      auto *l = new QLabel(label, card);
      Theme::setRole(l, "statLabel");
      grid->addWidget(l, 0, col);
      value = new QLabel("-", card);
      Theme::setRole(value, "statValue");
      grid->addWidget(value, 1, col);
    };
    makeStat(0, "COMPLETED", m_completedLabel);
    makeStat(1, "FAILED", m_failedLabel);
    makeStat(2, "ACTIVE", m_activeLabel);
    makeStat(3, "ELAPSED", m_elapsedLabel);
    makeStat(4, "REMAINING", m_etaLabel);
    for (int c = 0; c < 5; ++c)
      grid->setColumnStretch(c, 1);

    auto *right = new QVBoxLayout();
    right->setSpacing(12);
    right->addLayout(grid);

    m_headline = new QLabel(card);
    m_headline->setWordWrap(true);
    right->addWidget(m_headline);

    m_overallBar = new QProgressBar(card);
    m_overallBar->setRange(0, 100);
    m_overallBar->setTextVisible(false);
    m_overallBar->setProperty("thin", true);
    right->addWidget(m_overallBar);

    row->addLayout(right, 1);
    cl->addLayout(row);
    vl->addWidget(card);
  }

  // ---- Job list card -------------------------------------------------------
  {
    QVBoxLayout *cl = nullptr;
    auto *card = Theme::card(this, "JOB QUEUE", &cl);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);

    m_stack = new QStackedWidget(card);

    auto *empty = new QWidget(m_stack);
    auto *el = new QVBoxLayout(empty);
    el->setAlignment(Qt::AlignCenter);
    el->setSpacing(8);
    auto *ic = new QLabel(empty);
    ic->setPixmap(Theme::icon("progress", Theme::Color::TextFaint).pixmap(36, 36));
    ic->setAlignment(Qt::AlignCenter);
    el->addWidget(ic);
    auto *et = new QLabel("No batch running", empty);
    et->setObjectName("dropTitle");
    et->setAlignment(Qt::AlignCenter);
    el->addWidget(et);
    auto *eh = new QLabel("Add videos on the Input page and press Start "
                          "Processing.",
                          empty);
    eh->setObjectName("dropHint");
    eh->setAlignment(Qt::AlignCenter);
    el->addWidget(eh);
    el->addSpacing(6);
    auto *eb = new QPushButton("Go to Input", empty);
    eb->setObjectName("secondaryBtn");
    connect(eb, &QPushButton::clicked, this, &ProgressPanel::addFilesRequested);
    el->addWidget(eb, 0, Qt::AlignCenter);
    m_stack->addWidget(empty);

    m_jobList = new JobListWidget(m_stack);
    m_stack->addWidget(m_jobList);

    cl->addWidget(m_stack, 1);
    vl->addWidget(card, 1);
  }

  m_clock.setInterval(1000);
  connect(&m_clock, &QTimer::timeout, this, &ProgressPanel::updateElapsed);

  reset();
}

void ProgressPanel::setHeadline(const QString &text, const QString &role) {
  m_headline->setText(text);
  Theme::setRole(m_headline, role);
}

void ProgressPanel::setRunning(bool running) {
  m_running = running;
  m_pauseBtn->setEnabled(running);
  m_cancelBtn->setEnabled(running);
  if (running)
    m_clock.start();
  else
    m_clock.stop();
  if (!running) {
    m_paused = false;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setIcon(Theme::icon("pause", Theme::Color::Text));
  }
}

void ProgressPanel::setOutputDirectory(const QString &dir) {
  m_outputDir = dir;
  m_openBtn->setEnabled(!dir.isEmpty());
}

void ProgressPanel::reset() {
  m_totalJobs = 0;
  m_completedJobs = 0;
  m_failedJobs = 0;
  m_activePercent.clear();
  m_overallCircle->setValue(0);
  m_overallCircle->setAccentColor(Theme::Color::Accent);
  m_overallBar->setValue(0);
  m_overallBar->setProperty("state", "");
  Theme::repolish(m_overallBar);
  setHeadline("Idle - nothing is being processed.", "faint");
  m_headline->setToolTip(QString());
  m_completedLabel->setText("-");
  m_failedLabel->setText("-");
  m_activeLabel->setText("-");
  m_elapsedLabel->setText("-");
  m_etaLabel->setText("-");
  m_jobList->clearAll();
  m_stack->setCurrentIndex(0);
  setRunning(false);
}

void ProgressPanel::setTotalJobs(int count) {
  m_totalJobs = count;
  m_completedJobs = 0;
  m_failedJobs = 0;
  m_activePercent.clear();
  m_jobTimer.restart();
  setHeadline(
      QString("Processing %1 file%2...").arg(count).arg(count == 1 ? "" : "s"),
      "accent");
  m_completedLabel->setText("0");
  m_failedLabel->setText("0");
  m_activeLabel->setText("0");
  m_elapsedLabel->setText("0s");
  m_etaLabel->setText("...");
  m_stack->setCurrentIndex(1);
  setRunning(true);
}

void ProgressPanel::setJobs(const QList<Job> &jobs) {
  for (const Job &j : jobs)
    m_jobList->addJob(j);
  m_stack->setCurrentIndex(jobs.isEmpty() ? 0 : 1);
}

void ProgressPanel::onJobStarted(int jobId, const QString &filePath) {
  if (!m_jobList->hasJob(jobId)) {
    Job j;
    j.id = jobId;
    j.inputPath = filePath;
    m_jobList->addJob(j);
  }
  m_jobList->updateJobStatus(jobId, JobStatus::Extracting);
  m_activePercent.insert(jobId, 0);
  m_activeLabel->setText(QString::number(m_activePercent.size()));
}

void ProgressPanel::onJobProgress(int jobId, int percent,
                                  const QString &stage) {
  m_jobList->updateJobProgress(jobId, percent, stage);
  m_activePercent[jobId] = percent;
  updateOverallProgress();
}

void ProgressPanel::onJobFinished(int jobId, const QString &output) {
  m_completedJobs++;
  m_activePercent.remove(jobId);
  m_jobList->updateJobStatus(jobId, JobStatus::Completed);
  m_jobList->setJobOutput(jobId, output);
  updateOverallProgress();
}

void ProgressPanel::onJobFailed(int jobId, const QString &error) {
  m_failedJobs++;
  m_activePercent.remove(jobId);
  m_jobList->updateJobStatus(jobId, JobStatus::Failed);
  if (!error.isEmpty())
    m_jobList->setJobError(jobId, error);
  // Surface the most recent failure reason so the user does not have to open
  // the console to understand what went wrong.
  QString firstLine = error.section('\n', 0, 0).trimmed();
  if (!firstLine.isEmpty()) {
    setHeadline("Last failure: " + firstLine, "error");
    m_headline->setToolTip(error);
  }
  updateOverallProgress();
}

void ProgressPanel::onAllComplete() {
  setRunning(false);
  updateElapsed();
  if (m_failedJobs == 0)
    setHeadline(QString("All done - %1 file%2 subtitled.")
                    .arg(m_completedJobs)
                    .arg(m_completedJobs == 1 ? "" : "s"),
                "success");
  else
    setHeadline(QString("Finished - %1 completed, %2 failed. Hover a failed "
                        "row for details.")
                    .arg(m_completedJobs)
                    .arg(m_failedJobs),
                m_completedJobs == 0 ? "error" : "warning");
  m_activeLabel->setText("0");
  m_etaLabel->setText("-");
  m_overallCircle->setValue(100);
  m_overallCircle->setAccentColor(m_completedJobs == 0 && m_failedJobs > 0
                                      ? Theme::Color::Error
                                  : m_failedJobs > 0 ? Theme::Color::Warning
                                                     : Theme::Color::Success);
  m_overallBar->setValue(100);
  m_overallBar->setProperty("state", m_completedJobs == 0 && m_failedJobs > 0
                                         ? "error"
                                     : m_failedJobs > 0 ? "warn"
                                                        : "ok");
  Theme::repolish(m_overallBar);
}

void ProgressPanel::onCancelled() {
  m_jobList->cancelPending();
  m_pauseBtn->setEnabled(false);
  m_cancelBtn->setEnabled(false);
  m_etaLabel->setText("-");
  if (m_activePercent.isEmpty()) {
    setRunning(false);
    setHeadline("Cancelled.", "warning");
  } else {
    setHeadline("Cancelling - waiting for the running file to finish...",
                "warning");
  }
}

void ProgressPanel::updateElapsed() {
  if (!m_jobTimer.isValid())
    return;
  m_elapsedLabel->setText(
      Theme::formatDuration(static_cast<int>(m_jobTimer.elapsed() / 1000)));
}

void ProgressPanel::updateOverallProgress() {
  int done = m_completedJobs + m_failedJobs;
  m_completedLabel->setText(QString::number(m_completedJobs));
  m_failedLabel->setText(QString::number(m_failedJobs));
  m_activeLabel->setText(QString::number(m_activePercent.size()));

  if (m_totalJobs <= 0)
    return;

  // Finished jobs count as 100%, running jobs contribute their own percent.
  double work = done * 100.0;
  for (int pct : std::as_const(m_activePercent))
    work += pct;
  const double overall = qBound(0.0, work / m_totalJobs, 100.0);
  m_overallCircle->setValue(static_cast<int>(overall));
  m_overallBar->setValue(static_cast<int>(overall));

  if (done >= m_totalJobs)
    return;

  // Extrapolate from the fraction of total work done so far. Wait until one
  // job's worth of audio extraction (the 0-5% slice) is behind us and a few
  // seconds have passed, otherwise the estimate is wildly optimistic.
  const qint64 elapsed = m_jobTimer.elapsed();
  if (overall <= 5.0 / m_totalJobs || elapsed < 3000) {
    m_etaLabel->setText("...");
    return;
  }
  const int eta =
      static_cast<int>(elapsed / 1000.0 * (100.0 - overall) / overall);
  m_etaLabel->setText(Theme::formatDuration(eta));
}
