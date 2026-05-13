#pragma once
#include "Job.h"
#include <QMutex>
#include <QObject>
#include <QWaitCondition>
#include <deque>
#include <memory>


class JobQueue : public QObject {
  Q_OBJECT
public:
  explicit JobQueue(QObject *parent = nullptr);

  void enqueue(const Job &job);
  bool dequeue(Job &job);
  void pause();
  void resume();
  void cancelAll();

  int size() const;
  bool isEmpty() const;
  bool isPaused() const;
  QList<Job> allJobs() const;
  Job jobById(int id) const;
  void updateJob(const Job &job);

signals:
  void jobEnqueued(int jobId);
  void sizeChanged(int size);

private:
  mutable QMutex m_mutex;
  QWaitCondition m_notPaused;
  std::deque<Job> m_queue;
  QList<Job> m_allJobs;
  bool m_paused = false;
  bool m_cancelled = false;
};
