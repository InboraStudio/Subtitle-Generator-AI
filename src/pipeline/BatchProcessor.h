#pragma once
#include "JobQueue.h"
#include <QAtomicInt>
#include <QObject>
#include <QThreadPool>


class BatchProcessor : public QObject {
  Q_OBJECT
public:
  explicit BatchProcessor(JobQueue *queue, QObject *parent = nullptr);
  ~BatchProcessor();

  void start(int parallelCount = 1);
  void pause();
  void resume();
  void cancel();
  void setParallelCount(int count);

  bool isRunning() const;
  int activeJobCount() const;

signals:
  void jobStarted(int jobId, const QString &filePath);
  void jobProgress(int jobId, int percent, const QString &stage);
  void jobFinished(int jobId, const QString &outputPath);
  void jobFailed(int jobId, const QString &error);
  void allJobsCompleted();
  void processingCancelled();

private slots:
  void dispatchNext();

private:
  JobQueue *m_queue;
  QThreadPool m_pool;
  QAtomicInt m_activeCount;
  bool m_running = false;
  int m_parallelCount = 1;
};
