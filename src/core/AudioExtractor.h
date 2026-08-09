#pragma once
#include <QObject>
#include <QString>

class AudioExtractor : public QObject {
  Q_OBJECT
public:
  explicit AudioExtractor(const QString &ffmpegPath, QObject *parent = nullptr);

  bool extract(const QString &videoPath, const QString &outputWavPath);
  void cancel();

signals:
  void progress(int percent);
  void finished(const QString &wavPath);
  void failed(const QString &error);
  void logMessage(const QString &msg);

private:
  double probeDurationSeconds(const QString &videoPath) const;

  QString m_ffmpegPath;
  bool m_cancelled = false;
};
