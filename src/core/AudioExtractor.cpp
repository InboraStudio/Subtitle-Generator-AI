#include "AudioExtractor.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>


AudioExtractor::AudioExtractor(const QString &ffmpegPath, QObject *parent)
    : QObject(parent), m_ffmpegPath(ffmpegPath) {}

bool AudioExtractor::extract(const QString &videoPath,
                             const QString &outputWavPath) {
  m_cancelled = false;
  emit logMessage(
      QString("Extracting audio: %1").arg(QFileInfo(videoPath).fileName()));

  QDir().mkpath(QFileInfo(outputWavPath).absolutePath());

  QProcess proc;
  QStringList args = {"-y", "-i",   videoPath,   "-ar", "16000",      "-ac",
                      "1",  "-c:a", "pcm_s16le", "-vn", outputWavPath};

  proc.start(m_ffmpegPath, args);
  if (!proc.waitForStarted(5000)) {
    emit failed(QString("FFmpeg failed to start: %1").arg(proc.errorString()));
    return false;
  }

  emit progress(10);

  while (proc.state() == QProcess::Running) {
    if (m_cancelled) {
      proc.kill();
      emit failed("Extraction cancelled");
      return false;
    }
    proc.waitForFinished(200);
  }

  if (proc.exitCode() != 0) {
    QString err = QString::fromLocal8Bit(proc.readAllStandardError());
    emit failed(QString("FFmpeg error (code %1): %2")
                    .arg(proc.exitCode())
                    .arg(err.right(200)));
    return false;
  }

  if (!QFileInfo::exists(outputWavPath)) {
    emit failed("Output WAV not created");
    return false;
  }

  emit progress(100);
  emit finished(outputWavPath);
  emit logMessage("Audio extraction complete");
  return true;
}

void AudioExtractor::cancel() { m_cancelled = true; }
