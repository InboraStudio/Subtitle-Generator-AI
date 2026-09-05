#include "SystemMonitor.h"

#ifdef Q_OS_WIN
#include <pdh.h>
#include <psapi.h>
#include <windows.h>
#else
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStringList>
#endif

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {
  connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::poll);

#ifdef Q_OS_WIN
  PDH_HQUERY query = nullptr;
  PDH_HCOUNTER counter = nullptr;
  PdhOpenQuery(nullptr, 0, &query);
  PdhAddEnglishCounterW(query, L"\\Processor(_Total)\\% Processor Time", 0,
                        &counter);
  PdhCollectQueryData(query);
  m_cpuQuery = query;
  m_cpuCounter = counter;
#elifdef Q_OS_LINUX
  // Prime the /proc/stat delta
  queryCpu();
#endif
}

SystemMonitor::~SystemMonitor() {
  stopPolling();
#ifdef Q_OS_WIN
  if (m_cpuQuery)
    PdhCloseQuery(static_cast<PDH_HQUERY>(m_cpuQuery));
#endif
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

#ifdef Q_OS_LINUX
namespace {

QByteArray readFirstLine(const QString &path) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return {};
  return f.readLine().trimmed();
}

// Market name for the card, resolved once via lspci
QString friendlyGpuName(const QString &devicePath) {
  const QByteArray uevent = [&] {
    QFile f(devicePath + "/uevent");
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
  }();

  QString driver = QStringLiteral("gpu");
  QString pciId;
  for (const QByteArray &line : uevent.split('\n')) {
    if (line.startsWith("DRIVER="))
      driver = QString::fromLatin1(line.mid(7)).trimmed();
    else if (line.startsWith("PCI_ID="))
      pciId = QString::fromLatin1(line.mid(7)).trimmed();
  }

  if (!pciId.isEmpty()) {
    QProcess lspci;
    lspci.start("lspci", {"-mm", "-d", pciId});
    if (lspci.waitForFinished(1500) && lspci.exitCode() == 0) {
      // -mm quotes each field: slot class "Vendor" "Device" ...
      const QString out = QString::fromLocal8Bit(lspci.readAllStandardOutput());
      const QStringList parts = out.split('"');
      if (parts.size() >= 6) {
        // "die codename [market name]"
        const QString device = parts.at(5);
        const int open = device.lastIndexOf('[');
        const int close = device.lastIndexOf(']');
        if (open >= 0 && close > open)
          return device.mid(open + 1, close - open - 1);
        return device;
      }
    }
  }
  return QStringLiteral("GPU (%1)").arg(driver);
}

} // namespace
#endif

double SystemMonitor::queryCpu() {
#ifdef Q_OS_WIN
  if (!m_cpuQuery || !m_cpuCounter)
    return 0.0;

  PdhCollectQueryData(static_cast<PDH_HQUERY>(m_cpuQuery));
  PDH_FMT_COUNTERVALUE val;
  if (PdhGetFormattedCounterValue(static_cast<PDH_HCOUNTER>(m_cpuCounter),
                                  PDH_FMT_DOUBLE, nullptr,
                                  &val) == ERROR_SUCCESS)
    return qBound(0.0, val.doubleValue, 100.0);
  return 0.0;
#elifdef Q_OS_LINUX
  // "cpu  user nice system idle iowait irq softirq steal ..."
  const QByteArray line = readFirstLine(QStringLiteral("/proc/stat"));
  if (!line.startsWith("cpu "))
    return 0.0;

  const QList<QByteArray> f = line.simplified().split(' ');
  if (f.size() < 5)
    return 0.0;

  quint64 total = 0;
  for (int i = 1; i < f.size(); ++i)
    total += f.at(i).toULongLong();
  // idle + iowait count as not-busy.
  const quint64 idle = f.at(4).toULongLong() +
                       (f.size() > 5 ? f.at(5).toULongLong() : 0);

  const quint64 dTotal = total - m_prevTotal;
  const quint64 dIdle = idle - m_prevIdle;
  m_prevTotal = total;
  m_prevIdle = idle;

  if (dTotal == 0)
    return m_current.cpuPercent;
  return qBound(0.0, 100.0 * (dTotal - dIdle) / dTotal, 100.0);
#endif
}

double SystemMonitor::queryRam(qint64 &usedMB, qint64 &totalMB) {
#ifdef Q_OS_WIN
  MEMORYSTATUSEX ms;
  ms.dwLength = sizeof(ms);
  if (!GlobalMemoryStatusEx(&ms))
    return 0.0;

  totalMB = static_cast<qint64>(ms.ullTotalPhys / (1024 * 1024));
  qint64 freeMB = static_cast<qint64>(ms.ullAvailPhys / (1024 * 1024));
  usedMB = totalMB - freeMB;
  return (totalMB > 0) ? 100.0 * usedMB / totalMB : 0.0;
#elifdef Q_OS_LINUX
  QFile f(QStringLiteral("/proc/meminfo"));
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return 0.0;

  qint64 totalKB = 0, availKB = 0;
  forever {
    const QByteArray line = f.readLine();
    if (line.isEmpty())
      break;
    if (line.startsWith("MemTotal:"))
      totalKB = line.mid(9).simplified().split(' ').value(0).toLongLong();
    else if (line.startsWith("MemAvailable:"))
      availKB = line.mid(13).simplified().split(' ').value(0).toLongLong();
    if (totalKB && availKB)
      break;
  }
  if (totalKB <= 0)
    return 0.0;

  totalMB = totalKB / 1024;
  usedMB = (totalKB - availKB) / 1024;
  return qBound(0.0, 100.0 * usedMB / totalMB, 100.0);
#endif
}

double SystemMonitor::queryGpu(bool &available, QString &name) {
  available = false;
  name.clear();

#ifdef Q_OS_WIN
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
#elifdef Q_OS_LINUX
  if (!m_gpuInitialized) {
    m_gpuInitialized = true;
    const QStringList cards =
        QDir(QStringLiteral("/sys/class/drm"))
            .entryList({QStringLiteral("card[0-9]*")}, QDir::Dirs);
    for (const QString &card : cards) {
      // Skip the per-connector nodes (card1-DP-1 etc.)
      if (card.contains('-'))
        continue;
      const QString dev = QStringLiteral("/sys/class/drm/") + card + "/device";
      const QString busy = dev + "/gpu_busy_percent";
      if (QFile::exists(busy)) {
        m_gpuBusyPath = busy;
        m_gpuName = friendlyGpuName(dev);
        break;
      }
    }
  }

  if (m_gpuBusyPath.isEmpty())
    return 0.0;

  const QByteArray val = readFirstLine(m_gpuBusyPath);
  if (val.isEmpty())
    return 0.0;

  bool ok = false;
  const double pct = val.toDouble(&ok);
  if (!ok)
    return 0.0;

  available = true;
  name = m_gpuName;
  return qBound(0.0, pct, 100.0);
#endif
}
