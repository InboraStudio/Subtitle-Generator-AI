#include "BatchProcessor.h"
#include "ProcessingWorker.h"
#include <QMetaObject>

BatchProcessor::BatchProcessor(JobQueue *queue, QObject *parent)
    : QObject(parent), m_queue(queue) {
  m_pool.setMaxThreadCount(1);
}

BatchProcessor::~BatchProcessor() { m_pool.waitForDone(5000); }

void BatchProcessor::start(int parallelCount) {
  m_parallelCount = parallelCount;
  m_pool.setMaxThreadCount(parallelCount);
  m_running = true;
  dispatchNext();
}

void BatchProcessor::pause() { m_queue->pause(); }

void BatchProcessor::resume() {
  m_queue->resume();
  if (m_running)
    dispatchNext();
}

void BatchProcessor::cancel() {
  m_running = false;
  m_queue->cancelAll();
  emit processingCancelled();
}

void BatchProcessor::setParallelCount(int count) {
  m_parallelCount = count;
  m_pool.setMaxThreadCount(count);
}

bool BatchProcessor::isRunning() const { return m_running; }
int BatchProcessor::activeJobCount() const {
  return m_activeCount.loadAcquire();
}

void BatchProcessor::dispatchNext() {
  while (m_running && m_activeCount.loadAcquire() < m_parallelCount) {
    Job job;
    if (!m_queue->dequeue(job)) {
      if (m_activeCount.loadAcquire() == 0 && m_queue->isEmpty()) {
        m_running = false;
        emit allJobsCompleted();
      }
      return;
    }

    m_activeCount.fetchAndAddOrdered(1);
    ProcessingWorker *worker = new ProcessingWorker(job);
    worker->setAutoDelete(true);

    connect(worker, &ProcessingWorker::progress, this,
            &BatchProcessor::jobProgress, Qt::QueuedConnection);
    connect(
        worker, &ProcessingWorker::finished, this,
        [this](int id, const QString &out) {
          m_activeCount.fetchAndSubOrdered(1);
          emit jobFinished(id, out);
          QMetaObject::invokeMethod(this, "dispatchNext", Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    connect(
        worker, &ProcessingWorker::failed, this,
        [this](int id, const QString &err) {
          m_activeCount.fetchAndSubOrdered(1);
          emit jobFailed(id, err);
          QMetaObject::invokeMethod(this, "dispatchNext", Qt::QueuedConnection);
        },
        Qt::QueuedConnection);

    emit jobStarted(job.id, job.inputPath);
    m_pool.start(worker);
  }
}
