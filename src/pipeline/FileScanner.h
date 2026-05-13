#pragma once
#include <QDir>
#include <QObject>
#include <QStringList>


class FileScanner : public QObject {
  Q_OBJECT
public:
  explicit FileScanner(QObject *parent = nullptr);

  static QStringList scan(const QString &directory, bool recursive = true);
  static bool isVideoFile(const QString &path);
  static bool isValidVideoFile(const QString &path);
  static double getVideoDuration(const QString &ffprobePath,
                                 const QString &videoPath);

  static const QStringList &supportedExtensions();

signals:
  void scanProgress(int found);
  void scanFinished(const QStringList &files);
};
