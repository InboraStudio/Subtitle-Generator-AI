#pragma once
#include <QObject>
#include <QString>
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
  QString m_gpuName;

#ifdef Q_OS_WIN
  void *m_cpuQuery = nullptr;
  void *m_cpuCounter = nullptr;
#elifdef Q_OS_LINUX
  // Previous /proc/stat sample, so CPU load is a delta between polls.
  quint64 m_prevIdle = 0;
  quint64 m_prevTotal = 0;
  // sysfs node exposing GPU utilisation (amdgpu/i915)
  QString m_gpuBusyPath;
#endif
};
