#include "core/TranscriptionEngine.h"
#include "logging/Logger.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

int main(int argc, char *argv[]) {
  QApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  QApplication app(argc, argv);
  app.setApplicationName("SubtitleGeneratorAI");
  app.setApplicationVersion(APP_VERSION);
  app.setOrganizationName("SubtitleGeneratorAI");
  app.setWindowIcon(QIcon(":/icons/app.png"));
  app.setStyle("Fusion");
  Theme::apply(app);

  // Resolve command-line files before the working directory changes below.
  QStringList openPaths;
  for (const QString &arg : app.arguments().mid(1))
    openPaths << QFileInfo(arg).absoluteFilePath();

  QDir::setCurrent(QCoreApplication::applicationDirPath());

  QDir(TranscriptionEngine::userModelsDir()).mkpath(".");

  // Logs go to the user's AppData: the install folder may be read-only
  // (an all-users install under Program Files).
  const QString logDir =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
      "/logs";
  QDir(logDir).mkpath(".");
  Logger::instance()->setLogToFile(true, logDir + "/app.log");
  Logger::instance()->setLogLevel(LogLevel::Debug);

  MainWindow window;
  window.show();
  window.openPaths(openPaths);

  return app.exec();
}
