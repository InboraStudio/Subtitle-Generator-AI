#pragma once
#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <functional>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

struct LogEntry {
    QDateTime timestamp;
    LogLevel level;
    QString message;
    QString category;
};

class Logger : public QObject {
    Q_OBJECT
public:
    static Logger* instance();

    void setLogToFile(bool enabled, const QString& path = QString());
    void setLogLevel(LogLevel minLevel);
    void addSink(std::function<void(const LogEntry&)> sink);

    void debug(const QString& msg, const QString& cat = QStringLiteral("App"));
    void info(const QString& msg, const QString& cat = QStringLiteral("App"));
    void warning(const QString& msg, const QString& cat = QStringLiteral("App"));
    void error(const QString& msg, const QString& cat = QStringLiteral("App"));

signals:
    void entryAdded(const LogEntry& entry);

private:
    explicit Logger(QObject* parent = nullptr);
    ~Logger();

    void log(LogLevel level, const QString& msg, const QString& cat);
    static QString levelToString(LogLevel level);

    static Logger* s_instance;
    QMutex m_mutex;
    LogLevel m_minLevel = LogLevel::Debug;
    QFile m_logFile;
    QTextStream m_logStream;
    bool m_fileLogging = false;
    QList<std::function<void(const LogEntry&)>> m_sinks;
};

#define LOG_DEBUG(msg) Logger::instance()->debug(msg, __FUNCTION__)
#define LOG_INFO(msg) Logger::instance()->info(msg, __FUNCTION__)
#define LOG_WARN(msg) Logger::instance()->warning(msg, __FUNCTION__)
#define LOG_ERROR(msg) Logger::instance()->error(msg, __FUNCTION__)
