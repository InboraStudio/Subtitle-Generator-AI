#pragma once
#include "Logger.h"
#include <QAbstractListModel>
#include <QList>


class LogModel : public QAbstractListModel {
  Q_OBJECT
public:
  enum Roles {
    MessageRole = Qt::UserRole + 1,
    LevelRole,
    TimestampRole,
    CategoryRole,
    ColorRole
  };

  explicit LogModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setMaxEntries(int max);
  void clear();
  const QList<LogEntry> &entries() const;

public slots:
  void appendEntry(const LogEntry &entry);

private:
  QList<LogEntry> m_entries;
  int m_maxEntries = 2000;
};
