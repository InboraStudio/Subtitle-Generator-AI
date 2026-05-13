#pragma once
#include "../pipeline/Job.h"
#include <QObject>
#include <QString>

enum class EmbedMode { SoftSub, HardSub };

class VideoEmbedder : public QObject {
  Q_OBJECT
public:
  explicit VideoEmbedder(const QString &ffmpegPath, QObject *parent = nullptr);

  bool embed(const QString &videoPath, const QString &srtPath,
             const QString &outputPath, const Job &job,
             EmbedMode mode = EmbedMode::HardSub);
  void cancel();

signals:
  void progress(int percent);
  void finished(const QString &outputPath);
  void failed(const QString &error);
  void logMessage(const QString &msg);

private:
  QString m_ffmpegPath;
  bool m_cancelled = false;
};
