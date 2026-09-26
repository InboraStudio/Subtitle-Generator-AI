#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

struct TranscriptSegment {
  int index = 0;
  qint64 startMs = 0;
  qint64 endMs = 0;
  QString text;
};

class TranscriptionEngine : public QObject {
  Q_OBJECT
public:
  explicit TranscriptionEngine(QObject *parent = nullptr);
  ~TranscriptionEngine();

  bool loadModel(const QString &modelPath);
  bool transcribe(const QString &wavPath, QList<TranscriptSegment> &outSegments,
                  const QString &language = "auto", bool fastMode = false);
  void cancel();

  // Per-user, writable model folder outside the build/install tree, so
  // rebuilding or `make clean` never deletes downloaded models.
  static QString userModelsDir();
  // Read-only models bundled next to the executable (see CMake install()).
  static QString bundledModelsDir();
  static QStringList discoverModels(const QString &modelsDir);
  // Models from userModelsDir() then bundledModelsDir(); first name wins.
  static QStringList discoverModels();
  static QString modelTier(const QString &modelPath);
  bool isModelLoaded() const;
  void unloadModel();

signals:
  void progress(int percent);
  void segmentReady(const TranscriptSegment &segment);
  void logMessage(const QString &msg);

private:
  void *m_ctx = nullptr;
  bool m_cancelled = false;
};
