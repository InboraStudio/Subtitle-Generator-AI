#pragma once
#include "../../pipeline/Job.h"
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>

class JobListItem : public QWidget {
  Q_OBJECT
public:
  explicit JobListItem(const Job &job, QWidget *parent = nullptr);
  void updateProgress(int percent, const QString &stage);
  void setStatus(JobStatus status);
  void setError(const QString &error);
  void setOutput(const QString &path);
  int jobId() const;
  JobStatus status() const { return m_status; }

protected:
  void resizeEvent(QResizeEvent *e) override;

private:
  void elideName();

  int m_jobId;
  JobStatus m_status = JobStatus::Queued;
  QString m_fullName;
  QString m_outputPath;
  QLabel *m_nameLabel;
  QLabel *m_stageLabel;
  QLabel *m_percentLabel;
  QLabel *m_pill;
  QProgressBar *m_bar;
  QPushButton *m_showBtn;
};

class JobListWidget : public QWidget {
  Q_OBJECT
public:
  explicit JobListWidget(QWidget *parent = nullptr);

  void addJob(const Job &job);
  bool hasJob(int jobId) const;
  void updateJobProgress(int jobId, int percent, const QString &stage);
  void updateJobStatus(int jobId, JobStatus status);
  void setJobError(int jobId, const QString &error);
  void setJobOutput(int jobId, const QString &path);
  // Marks every job that never started as cancelled.
  void cancelPending();
  void clearAll();
  int jobCount() const;

signals:
  void removeJobRequested(int jobId);

private:
  QListWidget *m_list;
  QMap<int, QListWidgetItem *> m_itemMap;
  QMap<int, JobListItem *> m_widgetMap;
};
