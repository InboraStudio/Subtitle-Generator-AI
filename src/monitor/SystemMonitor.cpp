#include "SystemMonitor.h"
#include <pdh.h>
#include <psapi.h>
#include <windows.h>

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {
  connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::poll);

  PDH_HQUERY query = nullptr;
  PDH_HCOUNTER counter = nullptr;
  PdhOpenQuery(nullptr, 0, &query);
  PdhAddEnglishCounterW(query, L"\\Processor(_Total)\\% Processor Time", 0,
                        &counter);
  PdhCollectQueryData(query);
  m_cpuQuery = query;
  m_cpuCounter = counter;
}

SystemMonitor::~SystemMonitor() {
  stopPolling();
  if (m_cpuQuery)
    PdhCloseQuery(static_cast<PDH_HQUERY>(m_cpuQuery));
}

void SystemMonitor::startPolling(int intervalMs) { m_timer.start(intervalMs); }

void SystemMonitor::stopPolling() { m_timer.stop(); }

SystemStats SystemMonitor::currentStats() const { return m_current; }

void SystemMonitor::poll() {
  m_current.cpuPercent = queryCpu();
  m_current.ramPercent = queryRam(m_current.ramUsedMB, m_current.ramTotalMB);
  m_current.gpuPercent = queryGpu(m_current.gpuAvailable, m_current.gpuName);
  emit statsUpdated(m_current);
}

double SystemMonitor::queryCpu() {
  if (!m_cpuQuery || !m_cpuCounter)
    return 0.0;

  PdhCollectQueryData(static_cast<PDH_HQUERY>(m_cpuQuery));
  PDH_FMT_COUNTERVALUE val;
  if (PdhGetFormattedCounterValue(static_cast<PDH_HCOUNTER>(m_cpuCounter),
                                  PDH_FMT_DOUBLE, nullptr,
                                  &val) == ERROR_SUCCESS)
    return qBound(0.0, val.doubleValue, 100.0);
  return 0.0;
}

double SystemMonitor::queryRam(qint64 &usedMB, qint64 &totalMB) {
  MEMORYSTATUSEX ms;
  ms.dwLength = sizeof(ms);
  if (!GlobalMemoryStatusEx(&ms))
    return 0.0;

  totalMB = static_cast<qint64>(ms.ullTotalPhys / (1024 * 1024));
  qint64 freeMB = static_cast<qint64>(ms.ullAvailPhys / (1024 * 1024));
  usedMB = totalMB - freeMB;
  return (totalMB > 0) ? 100.0 * usedMB / totalMB : 0.0;
}

double SystemMonitor::queryGpu(bool &available, QString &name) {
  available = false;
  name.clear();

#if defined(GGML_USE_CUDA)
  if (!m_gpuInitialized) {
    if (nvmlInit() == NVML_SUCCESS) {
      m_gpuInitialized = true;
      char gpuName[NVML_DEVICE_NAME_BUFFER_SIZE];
      nvmlDevice_t dev;
      if (nvmlDeviceGetHandleByIndex(0, &dev) == NVML_SUCCESS) {
        nvmlDeviceGetName(dev, gpuName, sizeof(gpuName));
        m_gpuName = QString::fromLocal8Bit(gpuName);
      }
    }
  }
  if (m_gpuInitialized) {
    nvmlDevice_t dev;
    if (nvmlDeviceGetHandleByIndex(0, &dev) == NVML_SUCCESS) {
      nvmlUtilization_t util;
      if (nvmlDeviceGetUtilizationRates(dev, &util) == NVML_SUCCESS) {
        available = true;
        name = m_gpuName;
        return static_cast<double>(util.gpu);
      }
    }
  }
#endif
  return 0.0;
}
