#include "AudioExtractor.h"
#include "FfmpegLocator.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

AudioExtractor::AudioExtractor(const QString &ffmpegPath, QObject *parent)
    : QObject(parent), m_ffmpegPath(ffmpegPath) {}

// Probe the media duration (seconds) so extraction progress can be reported
// accurately. Returns a negative value when the duration is unavailable.
double AudioExtractor::probeDurationSeconds(const QString &videoPath) const {
  const QString ffprobe = FfmpegLocator::ffprobePath();
  if (ffprobe.isEmpty())
    return -1.0;

  QProcess proc;
  proc.start(ffprobe, {"-v", "error", "-show_entries", "format=duration",
                       "-of", "default=noprint_wrappers=1:nokey=1", videoPath});
  if (!proc.waitForStarted(3000))
    return -1.0;
  if (!proc.waitForFinished(8000)) {
    proc.kill();
    return -1.0;
  }
  bool ok = false;
  const double seconds =
      QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed().toDouble(&ok);
  return ok ? seconds : -1.0;
}

bool AudioExtractor::extract(const QString &videoPath,
                             const QString &outputWavPath) {
  m_cancelled = false;

  if (m_ffmpegPath.isEmpty()) {
    emit failed("FFmpeg executable not found. " + FfmpegLocator::installHint());
    return false;
  }
  if (!QFileInfo::exists(videoPath)) {
    emit failed(QString("Input file does not exist: %1").arg(videoPath));
    return false;
  }

  emit logMessage(
      QString("Extracting audio: %1").arg(QFileInfo(videoPath).fileName()));

  QDir().mkpath(QFileInfo(outputWavPath).absolutePath());

  const double totalSeconds = probeDurationSeconds(videoPath);

  QProcess proc;
  // ffmpeg prints progress ("time=HH:MM:SS.xx") to stderr; watch that channel.
  proc.setReadChannel(QProcess::StandardError);
  const QStringList args = {"-hide_banner", "-y",   "-i",  videoPath,
                            "-vn",          "-ar",  "16000", "-ac",
                            "1",            "-c:a", "pcm_s16le", outputWavPath};

  proc.start(m_ffmpegPath, args);
  if (!proc.waitForStarted(5000)) {
    emit failed(QString("FFmpeg failed to start (%1). Path: %2")
                    .arg(proc.errorString(), m_ffmpegPath));
    return false;
  }

  emit progress(1);

  static const QRegularExpression timeRe(
      QStringLiteral("time=(\\d+):(\\d+):(\\d+(?:\\.\\d+)?)"));
  QString stderrTail;

  while (proc.state() == QProcess::Running) {
    if (m_cancelled) {
      proc.kill();
      proc.waitForFinished(2000);
      QFile::remove(outputWavPath);
      emit failed("Extraction cancelled");
      return false;
    }
    proc.waitForReadyRead(200);
    const QString chunk = QString::fromLocal8Bit(proc.readAllStandardError());
    if (chunk.isEmpty())
      continue;
    stderrTail = (stderrTail + chunk).right(4000);

    if (totalSeconds > 0) {
      auto it = timeRe.globalMatch(chunk);
      double lastSeconds = -1.0;
      while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        lastSeconds = m.captured(1).toDouble() * 3600.0 +
                      m.captured(2).toDouble() * 60.0 + m.captured(3).toDouble();
      }
      if (lastSeconds >= 0.0) {
        const int pct =
            qBound(1, static_cast<int>(lastSeconds / totalSeconds * 100.0), 99);
        emit progress(pct);
      }
    }
  }

  // Drain anything left after the process exited.
  stderrTail = (stderrTail + QString::fromLocal8Bit(proc.readAllStandardError()))
                   .right(4000);

  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
    const QString reason = stderrTail.trimmed().isEmpty()
                               ? proc.errorString()
                               : stderrTail.trimmed().right(300);
    emit failed(QString("FFmpeg error (code %1): %2")
                    .arg(proc.exitCode())
                    .arg(reason));
    QFile::remove(outputWavPath);
    return false;
  }

  const QFileInfo out(outputWavPath);
  if (!out.exists() || out.size() <= 44) {
    emit failed("Extraction produced an empty audio file (no decodable audio "
                "track in the input?)");
    QFile::remove(outputWavPath);
    return false;
  }

  emit progress(100);
  emit finished(outputWavPath);
  emit logMessage("Audio extraction complete");
  return true;
}

void AudioExtractor::cancel() { m_cancelled = true; }
