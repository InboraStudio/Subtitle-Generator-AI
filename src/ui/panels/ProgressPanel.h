#pragma once
#include "../../pipeline/Job.h"
#include "../widgets/CircularProgressWidget.h"
#include "../widgets/JobListWidget.h"
#include <QElapsedTimer>
#include <QHash>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QWidget>

class ProgressPanel : public QWidget {
  Q_OBJECT
public:
  explicit ProgressPanel(QWidget *parent = nullptr);

  void reset();
  void setTotalJobs(int count);
  // Lists every job of the batch up front as "Queued".
  void setJobs(const QList<Job> &jobs);
  void setOutputDirectory(const QString &dir);

  int activeJobs() const { return m_activePercent.size(); }
  bool isRunning() const { return m_running; }

public slots:
  void onJobStarted(int jobId, const QString &filePath);
  void onJobProgress(int jobId, int percent, const QString &stage);
  void onJobFinished(int jobId, const QString &output);
  void onJobFailed(int jobId, const QString &error);
  void onAllComplete();
  void onCancelled();

signals:
  void pauseRequested(bool paused);
  void cancelRequested();
  void addFilesRequested();

private:
  void updateOverallProgress();
  void updateElapsed();
  void setRunning(bool running);
  void setHeadline(const QString &text, const QString &role);

  QStackedWidget *m_stack;
  JobListWidget *m_jobList;
  CircularProgressWidget *m_overallCircle;
  QProgressBar *m_overallBar;
  QLabel *m_headline;
  QLabel *m_completedLabel;
  QLabel *m_failedLabel;
  QLabel *m_activeLabel;
  QLabel *m_etaLabel;
  QLabel *m_elapsedLabel;
  QPushButton *m_pauseBtn;
  QPushButton *m_cancelBtn;
  QPushButton *m_openBtn;
  QTimer m_clock;

  QString m_outputDir;
  bool m_running = false;
  bool m_paused = false;
  int m_totalJobs = 0;
  int m_completedJobs = 0;
  int m_failedJobs = 0;
  QElapsedTimer m_jobTimer;
  // Latest percent of each job still running, for overall progress and ETA
  QHash<int, int> m_activePercent;
};
