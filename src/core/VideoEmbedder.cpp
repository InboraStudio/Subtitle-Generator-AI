#include "VideoEmbedder.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>

VideoEmbedder::VideoEmbedder(const QString &ffmpegPath, QObject *parent)
    : QObject(parent), m_ffmpegPath(ffmpegPath) {}

bool VideoEmbedder::embed(const QString &videoPath, const QString &srtPath,
                          const QString &outputPath, const Job &job,
                          EmbedMode mode) {
  m_cancelled = false;
  emit logMessage(QString("Embedding subtitles [%1]: %2")
                      .arg(mode == EmbedMode::HardSub ? "hard" : "soft")
                      .arg(QFileInfo(videoPath).fileName()));

  QDir().mkpath(QFileInfo(outputPath).absolutePath());

  QStringList args;
  args << "-y" << "-i" << videoPath;

  if (mode == EmbedMode::HardSub) {
    QString escapedSrt = srtPath;
    escapedSrt.replace("\\", "/").replace(":", "\\:");

    // Applying FFmpeg force_style for the subtitle aesthetic options
    QString forceStyle = QString("Fontname=%1,Fontsize=%2,PrimaryColour=&H00%3&"
                                 ",Outline=%4,Alignment=%5")
                             .arg(job.fontName)
                             .arg(job.fontSize)
                             .arg(job.fontColorHex)
                             .arg(job.outlineThickness)
                             .arg(job.alignment);

    args << "-vf"
         << QString("subtitles='%1':force_style='%2'")
                .arg(escapedSrt, forceStyle);
    args << "-c:v" << "libx264" << "-crf" << "18";
    args << "-c:a" << "aac" << "-b:a" << "192k";

    // Map all streams from the main input so audio/subtitles are preserved
    args << "-map" << "0";
  } else {
    args << "-i" << srtPath;
    args << "-c:v" << "copy" << "-c:a" << "aac" << "-b:a" << "192k";
    args << "-c:s" << "mov_text";
    args << "-map" << "0" << "-map" << "1";
  }

  args << outputPath;

  QProcess proc;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.start(m_ffmpegPath, args);

  if (!proc.waitForStarted(5000)) {
    emit failed(QString("FFmpeg failed to start: %1").arg(proc.errorString()));
    return false;
  }

  emit progress(10);
  while (proc.state() == QProcess::Running) {
    if (m_cancelled) {
      proc.kill();
      emit failed("Embedding cancelled");
      return false;
    }
    QString out = QString::fromLocal8Bit(proc.readAll());
    if (!out.isEmpty())
      emit logMessage(out.trimmed().right(120));
    proc.waitForFinished(300);
  }

  if (proc.exitCode() != 0) {
    QString err = QString::fromLocal8Bit(proc.readAll());
    emit failed(QString("FFmpeg embed failed (code %1)").arg(proc.exitCode()));
    emit logMessage(err.right(200));
    return false;
  }

  emit progress(100);
  emit finished(outputPath);
  emit logMessage(
      QString("Embedding complete: %1").arg(QFileInfo(outputPath).fileName()));
  return true;
}

void VideoEmbedder::cancel() { m_cancelled = true; }
