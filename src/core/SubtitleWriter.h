#pragma once
#include "TranscriptionEngine.h"
#include <QObject>
#include <QString>


class SubtitleWriter : public QObject {
  Q_OBJECT
public:
  explicit SubtitleWriter(QObject *parent = nullptr);

  bool writeSrt(const QList<TranscriptSegment> &segments,
                const QString &outputPath);

  static QString formatTimestamp(qint64 ms);
  static QList<TranscriptSegment> parseSrt(const QString &srtPath);

signals:
  void logMessage(const QString &msg);
};
