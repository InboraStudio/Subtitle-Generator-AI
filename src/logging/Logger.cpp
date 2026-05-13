#include "Logger.h"
#include <QMutexLocker>

Logger *Logger::s_instance = nullptr;

Logger *Logger::instance() {
  if (!s_instance)
    s_instance = new Logger();
  return s_instance;
}

Logger::Logger(QObject *parent) : QObject(parent) {}

Logger::~Logger() {
  if (m_fileLogging && m_logFile.isOpen())
    m_logFile.close();
}

void Logger::setLogToFile(bool enabled, const QString &path) {
  QMutexLocker lock(&m_mutex);
  m_fileLogging = enabled;
  if (enabled && !path.isEmpty()) {
    if (m_logFile.isOpen())
      m_logFile.close();
    m_logFile.setFileName(path);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append |
                       QIODevice::Text))
      m_logStream.setDevice(&m_logFile);
  }
}

void Logger::setLogLevel(LogLevel minLevel) {
  QMutexLocker lock(&m_mutex);
  m_minLevel = minLevel;
}

void Logger::addSink(std::function<void(const LogEntry &)> sink) {
  QMutexLocker lock(&m_mutex);
  m_sinks.append(sink);
}

void Logger::debug(const QString &msg, const QString &cat) {
  log(LogLevel::Debug, msg, cat);
}
void Logger::info(const QString &msg, const QString &cat) {
  log(LogLevel::Info, msg, cat);
}
void Logger::warning(const QString &msg, const QString &cat) {
  log(LogLevel::Warning, msg, cat);
}
void Logger::error(const QString &msg, const QString &cat) {
  log(LogLevel::Error, msg, cat);
}

void Logger::log(LogLevel level, const QString &msg, const QString &cat) {
  QMutexLocker lock(&m_mutex);
  if (level < m_minLevel)
    return;

  LogEntry entry;
  entry.timestamp = QDateTime::currentDateTime();
  entry.level = level;
  entry.message = msg;
  entry.category = cat;

  if (m_fileLogging && m_logFile.isOpen()) {
    m_logStream << entry.timestamp.toString(Qt::ISODate) << " ["
                << levelToString(level) << "] "
                << "[" << cat << "] " << msg << "\n";
    m_logStream.flush();
  }

  for (auto &sink : m_sinks)
    sink(entry);

  emit entryAdded(entry);
}

QString Logger::levelToString(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return QStringLiteral("DEBUG");
  case LogLevel::Info:
    return QStringLiteral("INFO");
  case LogLevel::Warning:
    return QStringLiteral("WARN");
  case LogLevel::Error:
    return QStringLiteral("ERROR");
  }
  return QStringLiteral("UNKNOWN");
}
