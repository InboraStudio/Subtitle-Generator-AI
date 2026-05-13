#include "JobQueue.h"
#include <QMutexLocker>

JobQueue::JobQueue(QObject *parent) : QObject(parent) {}

void JobQueue::enqueue(const Job &job) {
  QMutexLocker lock(&m_mutex);
  m_queue.push_back(job);
  m_allJobs.append(job);
  emit jobEnqueued(job.id);
  emit sizeChanged(static_cast<int>(m_queue.size()));
}

bool JobQueue::dequeue(Job &job) {
  QMutexLocker lock(&m_mutex);
  while (m_paused && !m_cancelled)
    m_notPaused.wait(&m_mutex);

  if (m_cancelled || m_queue.empty())
    return false;

  job = m_queue.front();
  m_queue.pop_front();
  emit sizeChanged(static_cast<int>(m_queue.size()));
  return true;
}

void JobQueue::pause() {
  QMutexLocker lock(&m_mutex);
  m_paused = true;
}

void JobQueue::resume() {
  QMutexLocker lock(&m_mutex);
  m_paused = false;
  m_notPaused.wakeAll();
}

void JobQueue::cancelAll() {
  QMutexLocker lock(&m_mutex);
  m_cancelled = true;
  m_paused = false;
  m_queue.clear();
  m_notPaused.wakeAll();
  emit sizeChanged(0);
}

int JobQueue::size() const {
  QMutexLocker lock(&m_mutex);
  return static_cast<int>(m_queue.size());
}

bool JobQueue::isEmpty() const {
  QMutexLocker lock(&m_mutex);
  return m_queue.empty();
}

bool JobQueue::isPaused() const {
  QMutexLocker lock(&m_mutex);
  return m_paused;
}

QList<Job> JobQueue::allJobs() const {
  QMutexLocker lock(&m_mutex);
  return m_allJobs;
}

Job JobQueue::jobById(int id) const {
  QMutexLocker lock(&m_mutex);
  for (const Job &j : m_allJobs)
    if (j.id == id)
      return j;
  return Job();
}

void JobQueue::updateJob(const Job &job) {
  QMutexLocker lock(&m_mutex);
  for (Job &j : m_allJobs) {
    if (j.id == job.id) {
      j = job;
      return;
    }
  }
}
