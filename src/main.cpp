#include "logging/Logger.h"
#include "ui/MainWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>


int main(int argc, char *argv[]) {
  QApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  QApplication app(argc, argv);
  app.setApplicationName("SubtitleGeneratorAI");
  app.setApplicationVersion("2.0");
  app.setOrganizationName("SubtitleGeneratorAI");
  app.setWindowIcon(QIcon(":/icons/app.png"));
  app.setStyle("Fusion");

  QDir::setCurrent(QCoreApplication::applicationDirPath());

  QDir(QCoreApplication::applicationDirPath() + "/models").mkpath(".");
  QDir(QCoreApplication::applicationDirPath() + "/bin").mkpath(".");
  QDir(QCoreApplication::applicationDirPath() + "/logs").mkpath(".");

  QString logPath = QCoreApplication::applicationDirPath() + "/logs/app.log";
  Logger::instance()->setLogToFile(true, logPath);
  Logger::instance()->setLogLevel(LogLevel::Debug);

  MainWindow window;
  window.show();

  return app.exec();
}
