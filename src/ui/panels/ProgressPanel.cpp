#include "ProgressPanel.h"
#include <QElapsedTimer>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

ProgressPanel::ProgressPanel(QWidget *parent) : QWidget(parent) {
  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(16, 16, 16, 16);
  vl->setSpacing(14);

  auto *title = new QLabel("Processing", this);
  title->setStyleSheet("color:#E0E0E0; font-size:10pt; font-weight:600;");
  vl->addWidget(title);

  auto *topCard = new QFrame(this);
  topCard->setObjectName("topCard");
  topCard->setStyleSheet("QFrame#topCard{background:#1C1C1C;border:1px solid "
                         "#262626;border-radius:8px;}");
  auto *tc = new QHBoxLayout(topCard);
  tc->setContentsMargins(16, 14, 16, 14);
  tc->setSpacing(20);

  m_overallCircle = new CircularProgressWidget(topCard);
  m_overallCircle->setThickness(10);
  m_overallCircle->setFixedSize(90, 90);
  tc->addWidget(m_overallCircle);

  auto *statsGrid = new QWidget(topCard);
  statsGrid->setObjectName("statsGrid");
  statsGrid->setStyleSheet("QWidget#statsGrid { background-color: transparent; }");
  auto *gl = new QVBoxLayout(statsGrid);
  gl->setContentsMargins(0, 0, 0, 0);
  gl->setSpacing(10);

  auto makeStat = [&](const QString &label, QLabel *&valLabel) {
    auto *row = new QHBoxLayout();
    auto *lbl = new QLabel(label, statsGrid);
    lbl->setStyleSheet("color:#707070; font-size:7.5pt; letter-spacing:0.8px; "
                       "font-weight:600;");
    lbl->setFixedWidth(90);
    row->addWidget(lbl);
    valLabel = new QLabel("-", statsGrid);
    valLabel->setStyleSheet("color:#E0E0E0; font-size:9pt; font-weight:600;");
    row->addWidget(valLabel);
    row->addStretch();
    gl->addLayout(row);
  };

  makeStat("COMPLETED", m_completedLabel);
  makeStat("FAILED", m_failedLabel);
  makeStat("ACTIVE", m_activeLabel);
  makeStat("ETA", m_etaLabel);
  tc->addWidget(statsGrid, 1);

  vl->addWidget(topCard);

  m_overallLabel = new QLabel("No jobs queued", this);
  m_overallLabel->setStyleSheet("color:#505050; font-size:8pt;");
  vl->addWidget(m_overallLabel);

  m_overallBar = new QProgressBar(this);
  m_overallBar->setRange(0, 100);
  m_overallBar->setValue(0);
  m_overallBar->setFixedHeight(6);
  m_overallBar->setTextVisible(false);
  vl->addWidget(m_overallBar);

  auto *queueTitle = new QLabel("Job Queue", this);
  queueTitle->setStyleSheet(
      "color:#808080; font-size:7.5pt; letter-spacing:0.5px; font-weight:600;");
  vl->addWidget(queueTitle);

  m_jobList = new JobListWidget(this);
  vl->addWidget(m_jobList, 1);
}

void ProgressPanel::reset() {
  m_totalJobs = 0;
  m_completedJobs = 0;
  m_failedJobs = 0;
  m_overallCircle->setValue(0);
  m_overallBar->setValue(0);
  m_overallLabel->setText("No jobs queued");
  m_overallLabel->setStyleSheet("color:#505050; font-size:8pt;");
  m_overallLabel->setToolTip(QString());
  m_completedLabel->setText("-");
  m_failedLabel->setText("-");
  m_activeLabel->setText("-");
  m_etaLabel->setText("-");
  m_jobList->clearAll();
}

void ProgressPanel::setTotalJobs(int count) {
  m_totalJobs = count;
  m_completedJobs = 0;
  m_failedJobs = 0;
  m_jobTimer.restart();
  m_overallLabel->setText(
      QString("Processing %1 file%2...").arg(count).arg(count == 1 ? "" : "s"));
  m_completedLabel->setText("0");
  m_failedLabel->setText("0");
  m_activeLabel->setText("0");
  m_etaLabel->setText("Calculating...");
}

void ProgressPanel::onJobStarted(int jobId, const QString &filePath) {
  Job j;
  j.id = jobId;
  j.inputPath = filePath;
  m_jobList->addJob(j);
  m_jobList->updateJobStatus(jobId, JobStatus::Extracting);
  m_activeLabel->setText(
      QString::number(m_jobList->jobCount() - m_completedJobs - m_failedJobs));
}

void ProgressPanel::onJobProgress(int jobId, int percent,
                                  const QString &stage) {
  m_jobList->updateJobProgress(jobId, percent, stage);
}

void ProgressPanel::onJobFinished(int jobId, const QString &) {
  m_completedJobs++;
  m_jobList->updateJobStatus(jobId, JobStatus::Completed);
  updateOverallProgress();
}

void ProgressPanel::onJobFailed(int jobId, const QString &error) {
  m_failedJobs++;
  m_jobList->updateJobStatus(jobId, JobStatus::Failed);
  if (!error.isEmpty())
    m_jobList->setJobError(jobId, error);
  // Surface the most recent failure reason above the queue so the user does not
  // have to open the console to understand what went wrong.
  QString firstLine = error.section('\n', 0, 0).trimmed();
  if (!firstLine.isEmpty()) {
    m_overallLabel->setText("Failed: " + firstLine);
    m_overallLabel->setStyleSheet("color:#E05050; font-size:8pt;");
    m_overallLabel->setToolTip(error);
  }
  updateOverallProgress();
}

void ProgressPanel::onAllComplete() {
  m_overallLabel->setText(QString("All done - %1 completed, %2 failed")
                              .arg(m_completedJobs)
                              .arg(m_failedJobs));
  m_overallLabel->setStyleSheet(
      m_failedJobs > 0 ? "color:#E0A030; font-size:8pt;"
                       : "color:#4CAF50; font-size:8pt;");
  m_activeLabel->setText("0");
  m_etaLabel->setText("-");
  m_overallCircle->setValue(100);
  m_overallBar->setValue(100);
}

void ProgressPanel::updateOverallProgress() {
  int done = m_completedJobs + m_failedJobs;
  m_completedLabel->setText(QString::number(m_completedJobs));
  m_failedLabel->setText(QString::number(m_failedJobs));

  if (m_totalJobs > 0) {
    int pct = done * 100 / m_totalJobs;
    m_overallCircle->setValue(pct);
    m_overallBar->setValue(pct);

    if (done > 0 && m_totalJobs > done) {
      qint64 elapsed = m_jobTimer.elapsed();
      double msPerJob = static_cast<double>(elapsed) / done;
      int remaining = m_totalJobs - done;
      double eta = msPerJob * remaining / 1000.0;
      if (eta < 60)
        m_etaLabel->setText(QString("%1s").arg(static_cast<int>(eta)));
      else
        m_etaLabel->setText(QString("%1m %2s")
                                .arg(static_cast<int>(eta / 60))
                                .arg(static_cast<int>(eta) % 60));
    }
  }
}
