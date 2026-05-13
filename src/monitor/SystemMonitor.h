#pragma once
#include <QObject>
#include <QTimer>
#include <atomic>

struct SystemStats {
  double cpuPercent = 0.0;
  double ramPercent = 0.0;
  double gpuPercent = 0.0;
  qint64 ramUsedMB = 0;
  qint64 ramTotalMB = 0;
  bool gpuAvailable = false;
  QString gpuName;
};

class SystemMonitor : public QObject {
  Q_OBJECT
public:
  explicit SystemMonitor(QObject *parent = nullptr);
  ~SystemMonitor();

  void startPolling(int intervalMs = 1000);
  void stopPolling();
  SystemStats currentStats() const;

signals:
  void statsUpdated(const SystemStats &stats);

private slots:
  void poll();

private:
  double queryCpu();
  double queryRam(qint64 &usedMB, qint64 &totalMB);
  double queryGpu(bool &available, QString &name);

  QTimer m_timer;
  SystemStats m_current;
  bool m_gpuInitialized = false;
  void *m_cpuQuery = nullptr;
  void *m_cpuCounter = nullptr;
};
