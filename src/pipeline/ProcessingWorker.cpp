#include "ProcessingWorker.h"
#include "../core/AudioExtractor.h"
#include "../core/SubtitleWriter.h"
#include "../core/TranscriptionEngine.h"
#include "../core/TranslationEngine.h"
#include "../core/VideoEmbedder.h"
#include "../logging/Logger.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>

QAtomicInt Job::s_idCounter(1);

ProcessingWorker::ProcessingWorker(const Job &job, QObject *parent)
    : QObject(parent), QRunnable(), m_job(job) {
  setAutoDelete(false);
}

void ProcessingWorker::run() {
  LOG_INFO(QString("Starting job #%1: %2")
               .arg(m_job.id)
               .arg(QFileInfo(m_job.inputPath).fileName()));

  QString tempDir =
      QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
      "/SubtitleGeneratorAI";
  QDir().mkpath(tempDir);

  QString tempWav = QString("%1/job_%2.wav").arg(tempDir).arg(m_job.id);

  emit progress(m_job.id, 5, "Extracting audio");
  if (!extractAudio(tempWav))
    return;
  if (m_cancelled) {
    emit failed(m_job.id, "Cancelled");
    return;
  }

  emit progress(m_job.id, 20, "Transcribing");
  QStringList srtLines;
  QList<TranscriptSegment> segments;
  {
    TranscriptionEngine engine;
    connect(&engine, &TranscriptionEngine::logMessage,
            [this](const QString &msg) { LOG_INFO(msg); });
    connect(&engine, &TranscriptionEngine::progress, [this](int p) {
      emit progress(m_job.id, 20 + p * 55 / 100, "Transcribing");
    });
    if (!engine.loadModel(m_job.modelPath)) {
      emit failed(m_job.id, "Could not load transcription model");
      QFile::remove(tempWav);
      return;
    }
    if (!engine.transcribe(tempWav, segments, m_job.sourceLanguage,
                           m_job.fastMode)) {
      emit failed(m_job.id, "Transcription failed");
      QFile::remove(tempWav);
      return;
    }
  }

  if (m_cancelled) {
    emit failed(m_job.id, "Cancelled");
    return;
  }

  if (m_job.enableTranslation && !m_job.targetLanguage.isEmpty()) {
    emit progress(m_job.id, 76, "Translating");
    TranslationEngine translator;
    connect(&translator, &TranslationEngine::logMessage,
            [this](const QString &msg) { LOG_INFO(msg); });
    translator.translate(segments, m_job.sourceLanguage, m_job.targetLanguage);
  }

  emit progress(m_job.id, 88, "Writing SRT");
  {
    SubtitleWriter writer;
    connect(&writer, &SubtitleWriter::logMessage,
            [this](const QString &msg) { LOG_INFO(msg); });
    if (!writer.writeSrt(segments, m_job.outputSrtPath)) {
      emit failed(m_job.id,
                  QString("Failed to write SRT: %1").arg(m_job.outputSrtPath));
      QFile::remove(tempWav);
      return;
    }
  }

  if (m_job.embedSubtitles && !m_job.outputVideoPath.isEmpty()) {
    QString ffmpegPath =
        QCoreApplication::applicationDirPath() + "/bin/ffmpeg.exe";
    if (!QFileInfo::exists(ffmpegPath)) {
      ffmpegPath = "ffmpeg";
    }
    VideoEmbedder embedder(ffmpegPath);
    connect(&embedder, &VideoEmbedder::logMessage,
            [this](const QString &msg) { LOG_INFO(msg); });
    connect(&embedder, &VideoEmbedder::progress, [this](int p) {
      emit progress(m_job.id, 92 + p * 7 / 100, "Embedding");
    });
    if (m_job.embedSubtitles) {
      emit progress(m_job.id, 90, "Embedding Hard Subtitles");
      embedder.embed(m_job.inputPath, m_job.outputSrtPath,
                     m_job.outputVideoPath, m_job, EmbedMode::HardSub);
    } else {
      // Generate soft sub (muxing tracks)
      emit progress(m_job.id, 90, "Embedding Soft Subtitles");
      embedder.embed(m_job.inputPath, m_job.outputSrtPath,
                     m_job.outputVideoPath, m_job, EmbedMode::SoftSub);
    }
  }

  QFile::remove(tempWav);
  emit progress(m_job.id, 100, "Complete");
  emit finished(m_job.id, m_job.outputSrtPath);
  LOG_INFO(QString("Job #%1 finished").arg(m_job.id));
}

bool ProcessingWorker::extractAudio(const QString &tempWav) {
  QString ffmpegPath =
      QCoreApplication::applicationDirPath() + "/bin/ffmpeg.exe";
  if (!QFileInfo::exists(ffmpegPath)) {
    ffmpegPath = "ffmpeg";
  }
  AudioExtractor extractor(ffmpegPath);
  connect(&extractor, &AudioExtractor::logMessage,
          [this](const QString &msg) { LOG_INFO(msg); });
  connect(&extractor, &AudioExtractor::progress, [this](int p) {
    emit progress(m_job.id, p / 5, "Extracting audio");
  });
  if (!extractor.extract(m_job.inputPath, tempWav)) {
    emit failed(m_job.id, "Audio extraction failed");
    return false;
  }
  return true;
}
