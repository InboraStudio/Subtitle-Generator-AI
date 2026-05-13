#include "TranscriptionEngine.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QThread>

#ifdef WHISPER_AVAILABLE
#include "whisper.h"
#endif

TranscriptionEngine::TranscriptionEngine(QObject *parent) : QObject(parent) {}

TranscriptionEngine::~TranscriptionEngine() { unloadModel(); }

bool TranscriptionEngine::isModelLoaded() const { return m_ctx != nullptr; }

void TranscriptionEngine::unloadModel() {
#ifdef WHISPER_AVAILABLE
  if (m_ctx) {
    whisper_free(static_cast<whisper_context *>(m_ctx));
    m_ctx = nullptr;
  }
#endif
}

bool TranscriptionEngine::loadModel(const QString &modelPath) {
  unloadModel();
  emit logMessage(
      QString("Loading model: %1").arg(QFileInfo(modelPath).fileName()));

#ifdef WHISPER_AVAILABLE
  whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = true;
  m_ctx = whisper_init_from_file_with_params(
      modelPath.toLocal8Bit().constData(), cparams);
  if (!m_ctx) {
    emit logMessage("Failed to load model - retrying without GPU");
    cparams.use_gpu = false;
    m_ctx = whisper_init_from_file_with_params(
        modelPath.toLocal8Bit().constData(), cparams);
  }
  if (!m_ctx) {
    emit logMessage("ERROR: Could not load whisper model");
    return false;
  }
  emit logMessage("Model loaded successfully");
  return true;
#else
  emit logMessage("WARNING: whisper.cpp not compiled in - using stub output");
  m_ctx = reinterpret_cast<void *>(0x1);
  return true;
#endif
}

bool TranscriptionEngine::transcribe(const QString &wavPath,
                                     QList<TranscriptSegment> &outSegments,
                                     const QString &language, bool fastMode) {
  m_cancelled = false;

  if (!m_ctx) {
    emit logMessage("ERROR: No model loaded");
    return false;
  }

  emit logMessage(QString("Transcribing: %1 [lang=%2, fast=%3]")
                      .arg(QFileInfo(wavPath).fileName())
                      .arg(language)
                      .arg(fastMode ? "yes" : "no"));

#ifdef WHISPER_AVAILABLE
  QFile f(wavPath);
  if (!f.open(QIODevice::ReadOnly)) {
    emit logMessage("ERROR: Cannot open WAV file");
    return false;
  }
  QByteArray data = f.readAll();
  f.close();

  const int headerSize = 44;
  if (data.size() <= headerSize) {
    emit logMessage("ERROR: WAV file too small");
    return false;
  }

  // The rest of this is not needed since we read int16_t below

  const int16_t *raw16 = reinterpret_cast<const int16_t *>(data.constData() + headerSize);
  int nSamples16 = (data.size() - headerSize) / sizeof(int16_t);

  std::vector<float> pcmf32(nSamples16);
  for (int i = 0; i < nSamples16; i++)
    pcmf32[i] = static_cast<float>(raw16[i]) / 32768.0f;

  whisper_full_params wparams = whisper_full_default_params(
      fastMode ? WHISPER_SAMPLING_GREEDY : WHISPER_SAMPLING_BEAM_SEARCH);

  wparams.print_progress = false;
  wparams.print_realtime = false;
  wparams.print_special = false;
  wparams.translate = false;
  wparams.language = (language == "auto" || language.isEmpty())
                         ? nullptr
                         : language.toLocal8Bit().constData();
  wparams.n_threads = qMax(1, QThread::idealThreadCount() - 1);
  wparams.beam_search.beam_size = fastMode ? 1 : 5;

  whisper_context *ctx = static_cast<whisper_context *>(m_ctx);
  if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
    emit logMessage("ERROR: whisper_full failed");
    return false;
  }

  int nSeg = whisper_full_n_segments(ctx);
  for (int i = 0; i < nSeg; i++) {
    if (m_cancelled)
      return false;
    TranscriptSegment seg;
    seg.index = i + 1;
    seg.startMs = whisper_full_get_segment_t0(ctx, i) * 10;
    seg.endMs = whisper_full_get_segment_t1(ctx, i) * 10;
    seg.text =
        QString::fromUtf8(whisper_full_get_segment_text(ctx, i)).trimmed();
    outSegments.append(seg);
    emit segmentReady(seg);
    emit progress(static_cast<int>(100.0 * i / nSeg));
  }

  emit progress(100);
  emit logMessage(
      QString("Transcription complete: %1 segments").arg(outSegments.size()));
  return true;
#else
  TranscriptSegment stub;
  stub.index = 1;
  stub.startMs = 0;
  stub.endMs = 3000;
  stub.text = "[whisper.cpp not available - stub output]";
  outSegments.append(stub);
  emit progress(100);
  emit logMessage("Stub transcription complete (no whisper.cpp)");
  return true;
#endif
}

void TranscriptionEngine::cancel() { m_cancelled = true; }

QStringList TranscriptionEngine::discoverModels(const QString &modelsDir) {
  QStringList models;
  QDirIterator it(modelsDir, {"*.bin", "*.gguf"}, QDir::Files);
  while (it.hasNext())
    models.append(it.next());
  return models;
}

QString TranscriptionEngine::modelTier(const QString &modelPath) {
  QString name = QFileInfo(modelPath).baseName().toLower();
  if (name.contains("tiny"))
    return "Fast";
  if (name.contains("base"))
    return "Fast";
  if (name.contains("small"))
    return "Balanced";
  if (name.contains("medium"))
    return "Accurate";
  if (name.contains("large"))
    return "Accurate";
  return "Unknown";
}
