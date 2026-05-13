#include "MonitorWorker.h"
#include <QEventLoop>

MonitorWorker::MonitorWorker(QObject *parent) : QThread(parent) {}

MonitorWorker::~MonitorWorker() {
  stopWorker();
  wait(3000);
}

void MonitorWorker::stopWorker() {
  m_stop = true;
  if (m_monitor)
    QMetaObject::invokeMethod(
        m_monitor, [this] { m_monitor->stopPolling(); }, Qt::QueuedConnection);
  quit();
}

SystemMonitor *MonitorWorker::monitor() const { return m_monitor; }

void MonitorWorker::run() {
  m_monitor = new SystemMonitor();
  connect(m_monitor, &SystemMonitor::statsUpdated, this,
          &MonitorWorker::statsUpdated);
  m_monitor->startPolling(1000);
  exec();
  delete m_monitor;
  m_monitor = nullptr;
}
