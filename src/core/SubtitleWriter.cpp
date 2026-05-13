#include "SubtitleWriter.h"
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>


SubtitleWriter::SubtitleWriter(QObject *parent) : QObject(parent) {}

bool SubtitleWriter::writeSrt(const QList<TranscriptSegment> &segments,
                              const QString &outputPath) {
  QFile file(outputPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    emit logMessage(QString("ERROR: Cannot write SRT: %1").arg(outputPath));
    return false;
  }

  QTextStream out(&file);
  out.setEncoding(QStringConverter::Utf8);

  for (const TranscriptSegment &seg : segments) {
    out << seg.index << "\n";
    out << formatTimestamp(seg.startMs) << " --> " << formatTimestamp(seg.endMs)
        << "\n";
    out << seg.text.trimmed() << "\n\n";
  }

  file.close();
  emit logMessage(QString("SRT written: %1 (%2 segments)")
                      .arg(outputPath)
                      .arg(segments.size()));
  return true;
}

QString SubtitleWriter::formatTimestamp(qint64 ms) {
  qint64 totalSeconds = ms / 1000;
  qint64 millis = ms % 1000;
  qint64 seconds = totalSeconds % 60;
  qint64 minutes = (totalSeconds / 60) % 60;
  qint64 hours = totalSeconds / 3600;
  return QString("%1:%2:%3,%4")
      .arg(hours, 2, 10, QChar('0'))
      .arg(minutes, 2, 10, QChar('0'))
      .arg(seconds, 2, 10, QChar('0'))
      .arg(millis, 3, 10, QChar('0'));
}

QList<TranscriptSegment> SubtitleWriter::parseSrt(const QString &srtPath) {
  QList<TranscriptSegment> result;
  QFile file(srtPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return result;

  QTextStream in(&file);
  static QRegularExpression timeRe(
      R"((\d{2}):(\d{2}):(\d{2}),(\d{3}) --> (\d{2}):(\d{2}):(\d{2}),(\d{3}))");
  static QRegularExpression indexRe(R"(^\d+$)");

  TranscriptSegment current;
  bool inBlock = false;

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty()) {
      if (inBlock && !current.text.isEmpty()) {
        result.append(current);
        current = TranscriptSegment();
        inBlock = false;
      }
      continue;
    }

    auto m = timeRe.match(line);
    if (m.hasMatch()) {
      auto toMs = [](int h, int min, int s, int ms) -> qint64 {
        return h * 3600000LL + min * 60000LL + s * 1000LL + ms;
      };
      current.startMs = toMs(m.captured(1).toInt(), m.captured(2).toInt(),
                             m.captured(3).toInt(), m.captured(4).toInt());
      current.endMs = toMs(m.captured(5).toInt(), m.captured(6).toInt(),
                           m.captured(7).toInt(), m.captured(8).toInt());
      inBlock = true;
      continue;
    }

    if (indexRe.match(line).hasMatch() && !inBlock) {
      current.index = line.toInt();
      continue;
    }

    if (inBlock)
      current.text += (current.text.isEmpty() ? "" : "\n") + line;
  }

  if (!current.text.isEmpty())
    result.append(current);

  return result;
}
