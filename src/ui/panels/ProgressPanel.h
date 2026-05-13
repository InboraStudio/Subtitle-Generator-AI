#pragma once
#include "../widgets/CircularProgressWidget.h"
#include "../widgets/JobListWidget.h"
#include <QElapsedTimer>
#include <QLabel>
#include <QProgressBar>
#include <QWidget>

class ProgressPanel : public QWidget {
  Q_OBJECT
public:
  explicit ProgressPanel(QWidget *parent = nullptr);

  void reset();
  void setTotalJobs(int count);

public slots:
  void onJobStarted(int jobId, const QString &filePath);
  void onJobProgress(int jobId, int percent, const QString &stage);
  void onJobFinished(int jobId, const QString &output);
  void onJobFailed(int jobId, const QString &error);
  void onAllComplete();

private:
  void updateOverallProgress();

  JobListWidget *m_jobList;
  CircularProgressWidget *m_overallCircle;
  QProgressBar *m_overallBar;
  QLabel *m_overallLabel;
  QLabel *m_completedLabel;
  QLabel *m_failedLabel;
  QLabel *m_activeLabel;
  QLabel *m_etaLabel;

  int m_totalJobs = 0;
  int m_completedJobs = 0;
  int m_failedJobs = 0;
  QElapsedTimer m_jobTimer;
};
