#pragma once
#include <QAtomicInt>
#include <QString>
#include <QStringList>

enum class JobStatus {
  Queued,
  Extracting,
  Transcribing,
  Translating,
  Writing,
  Embedding,
  Completed,
  Failed,
  Cancelled
};

struct Job {
  int id = 0;
  QString inputPath;
  QString outputDirectory;
  QString outputSrtPath;
  QString outputVideoPath;
  QString modelPath;
  QString sourceLanguage;
  QString targetLanguage;
  bool enableTranslation = false;
  bool embedSubtitles = false;
  bool fastMode = false;

  // Subtitle styling options
  QString fontName = "Arial";
  int fontSize = 24;
  QString fontColorHex = "FFFFFF";
  int outlineThickness = 2;
  int alignment = 2; // 2=BottomCenter

  JobStatus status = JobStatus::Queued;
  int progressPercent = 0;
  QString errorMessage;
  double durationSeconds = 0.0;
  QStringList subtitleLines;

  static QAtomicInt s_idCounter;
  Job() : id(s_idCounter.fetchAndAddOrdered(1)) {}
};
