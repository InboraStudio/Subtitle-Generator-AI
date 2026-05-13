#include "LogModel.h"
#include <QColor>

LogModel::LogModel(QObject *parent) : QAbstractListModel(parent) {}

int LogModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_entries.size();
}

QVariant LogModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_entries.size())
    return QVariant();

  const LogEntry &e = m_entries.at(index.row());
  switch (role) {
  case Qt::DisplayRole:
  case MessageRole: {
    return QString("[%1] [%2] %3")
        .arg(e.timestamp.toString("HH:mm:ss"))
        .arg(e.category)
        .arg(e.message);
  }
  case LevelRole:
    return static_cast<int>(e.level);
  case TimestampRole:
    return e.timestamp;
  case CategoryRole:
    return e.category;
  case ColorRole: {
    switch (e.level) {
    case LogLevel::Debug:
      return QColor("#505060");
    case LogLevel::Info:
      return QColor("#C8C8C8");
    case LogLevel::Warning:
      return QColor("#E8872A");
    case LogLevel::Error:
      return QColor("#E05050");
    }
    return QColor("#C8C8C8");
  }
  }
  return QVariant();
}

QHash<int, QByteArray> LogModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[MessageRole] = "message";
  roles[LevelRole] = "level";
  roles[TimestampRole] = "timestamp";
  roles[CategoryRole] = "category";
  roles[ColorRole] = "color";
  return roles;
}

void LogModel::setMaxEntries(int max) { m_maxEntries = max; }

void LogModel::clear() {
  beginResetModel();
  m_entries.clear();
  endResetModel();
}

const QList<LogEntry> &LogModel::entries() const { return m_entries; }

void LogModel::appendEntry(const LogEntry &entry) {
  if (m_entries.size() >= m_maxEntries) {
    beginRemoveRows(QModelIndex(), 0, 0);
    m_entries.removeFirst();
    endRemoveRows();
  }
  beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
  m_entries.append(entry);
  endInsertRows();
}
