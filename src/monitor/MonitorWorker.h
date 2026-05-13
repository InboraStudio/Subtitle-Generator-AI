#pragma once
#include "SystemMonitor.h"
#include <QThread>


class MonitorWorker : public QThread {
  Q_OBJECT
public:
  explicit MonitorWorker(QObject *parent = nullptr);
  ~MonitorWorker();

  void stopWorker();
  SystemMonitor *monitor() const;

signals:
  void statsUpdated(const SystemStats &stats);

protected:
  void run() override;

private:
  SystemMonitor *m_monitor = nullptr;
  std::atomic<bool> m_stop{false};
};
