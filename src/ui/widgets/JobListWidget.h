#pragma once
#include "../../pipeline/Job.h"
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QProgressBar>
#include <QWidget>

class JobListItem : public QWidget {
  Q_OBJECT
public:
  explicit JobListItem(const Job &job, QWidget *parent = nullptr);
  void updateProgress(int percent, const QString &stage);
  void setStatus(JobStatus status);
  int jobId() const;

private:
  int m_jobId;
  QLabel *m_nameLabel;
  QLabel *m_statusLabel;
  QLabel *m_stageLabel;
  QProgressBar *m_bar;
};

class JobListWidget : public QWidget {
  Q_OBJECT
public:
  explicit JobListWidget(QWidget *parent = nullptr);

  void addJob(const Job &job);
  void updateJobProgress(int jobId, int percent, const QString &stage);
  void updateJobStatus(int jobId, JobStatus status);
  void clearAll();
  int jobCount() const;

signals:
  void removeJobRequested(int jobId);

private:
  QListWidget *m_list;
  QMap<int, QListWidgetItem *> m_itemMap;
  QMap<int, JobListItem *> m_widgetMap;
};
