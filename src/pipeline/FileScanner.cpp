#include "FileScanner.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

FileScanner::FileScanner(QObject *parent) : QObject(parent) {}

const QStringList &FileScanner::supportedExtensions() {
  static QStringList ext = {"mp4", "mkv", "avi", "mov",  "webm", "flv", "wmv",    
                            "m4v", "ts",  "mts", "m2ts", "mpg",  "mpeg"};          
  return ext;
}

bool FileScanner::isVideoFile(const QString &path) {
  QFileInfo fi(path);
  return supportedExtensions().contains(fi.suffix().toLower());
}

bool FileScanner::isValidVideoFile(const QString &path) {
  QFileInfo fi(path);
  return fi.exists() && fi.isFile() && fi.size() > 1024 && isVideoFile(path);
}

QStringList FileScanner::scan(const QString &directory, bool recursive) {
  QStringList result;
  QDir dir(directory);
  if (!dir.exists())
    return result;

  QDir::Filters filters = QDir::Files;
  QDirIterator::IteratorFlags flags =
      recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;

  QDirIterator it(directory, filters, flags);
  while (it.hasNext()) {
    QString path = it.next();
    if (isVideoFile(path))
      result.append(path);
  }
  return result;
}

double FileScanner::getVideoDuration(const QString &ffprobePath,
                                     const QString &videoPath) {
  QProcess proc;
  QStringList args = {"-v",   "quiet",        "-print_format",
                      "json", "-show_format", videoPath};
  proc.start(ffprobePath, args);
  if (!proc.waitForFinished(10000))
    return 0.0;

  QByteArray output = proc.readAllStandardOutput();
  QJsonDocument doc = QJsonDocument::fromJson(output);
  if (doc.isNull())
    return 0.0;

  QJsonObject format = doc.object().value("format").toObject();
  QString dur = format.value("duration").toString();
  return dur.toDouble();
}
