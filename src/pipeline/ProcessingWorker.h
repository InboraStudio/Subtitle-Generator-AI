#pragma once
#include "Job.h"
#include <QObject>
#include <QRunnable>


class ProcessingWorker : public QObject, public QRunnable {
  Q_OBJECT
public:
  explicit ProcessingWorker(const Job &job, QObject *parent = nullptr);
  void run() override;

signals:
  void progress(int jobId, int percent, const QString &stage);
  void finished(int jobId, const QString &outputPath);
  void failed(int jobId, const QString &error);

private:
  bool extractAudio(const QString &tempWav);
  bool transcribe(const QString &wavPath, QStringList &segments);
  bool translate(QStringList &segments);
  bool writeSrt(const QStringList &segments);
  bool embedSubtitles();

  Job m_job;
  bool m_cancelled = false;
};
