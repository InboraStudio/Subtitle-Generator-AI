#include "FfmpegLocator.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace {

#ifdef Q_OS_WIN
constexpr const char *kExeSuffix = ".exe";
#else
constexpr const char *kExeSuffix = "";
#endif

QString withSuffix(const QString &base) {
  return base + QString::fromLatin1(kExeSuffix);
}

// Candidate directories to probe, in priority order, for the given tool.
QStringList candidateDirectories() {
  QStringList dirs;
  const QString appDir = QCoreApplication::applicationDirPath();
  dirs << appDir + "/bin" << appDir;

#ifdef Q_OS_WIN
  // MSYS2 toolchains (the environment this app is commonly built in).
  const QStringList msysRoots = {"C:/msys64", "C:/msys32"};
  const QStringList msysEnvs = {"ucrt64", "mingw64", "clang64", "mingw32"};
  for (const QString &root : msysRoots)
    for (const QString &env : msysEnvs)
      dirs << QString("%1/%2/bin").arg(root, env);

  // Common per-user / system package managers.
  const QString localAppData =
      QProcessEnvironment::systemEnvironment().value("LOCALAPPDATA");
  if (!localAppData.isEmpty()) {
    // winget shims and Gyan/BtbN full builds unpacked under Packages.
    dirs << localAppData + "/Microsoft/WinGet/Links";
  }
  dirs << "C:/ProgramData/chocolatey/bin";
  const QString userProfile =
      QProcessEnvironment::systemEnvironment().value("USERPROFILE");
  if (!userProfile.isEmpty())
    dirs << userProfile + "/scoop/shims";

  // Typical manual-install locations.
  dirs << "C:/ffmpeg/bin" << "C:/Program Files/ffmpeg/bin";
#else
  dirs << "/usr/local/bin" << "/usr/bin" << "/opt/homebrew/bin"
       << "/snap/bin";
#endif
  return dirs;
}

// Resolve a tool ("ffmpeg" / "ffprobe") to an absolute, executable path.
QString locate(const QString &tool) {
  const QString exe = withSuffix(tool);

  // 1 & 3: explicit candidate directories.
  for (const QString &dir : candidateDirectories()) {
    QFileInfo fi(QDir(dir).absoluteFilePath(exe));
    if (fi.exists() && fi.isFile())
      return fi.absoluteFilePath();
  }

  // 2: anything already resolvable on PATH.
  const QString onPath = QStandardPaths::findExecutable(tool);
  if (!onPath.isEmpty())
    return onPath;

#ifdef Q_OS_WIN
  // As a last resort search the WinGet package tree, where Gyan.FFmpeg unpacks
  // ffmpeg.exe several directories deep under a versioned folder.
  const QString localAppData =
      QProcessEnvironment::systemEnvironment().value("LOCALAPPDATA");
  if (!localAppData.isEmpty()) {
    QDir pkgRoot(localAppData + "/Microsoft/WinGet/Packages");
    if (pkgRoot.exists()) {
      const QStringList pkgs =
          pkgRoot.entryList({"*FFmpeg*", "*ffmpeg*"}, QDir::Dirs | QDir::NoDotAndDotDot);
      for (const QString &pkg : pkgs) {
        QDirIterator it(pkgRoot.absoluteFilePath(pkg), {exe}, QDir::Files,
                        QDirIterator::Subdirectories);
        if (it.hasNext())
          return it.next();
      }
    }
  }
#endif

  return QString();
}

QString g_ffmpeg;
QString g_ffprobe;
bool g_ffmpegResolved = false;
bool g_ffprobeResolved = false;

} // namespace

namespace FfmpegLocator {

QString ffmpegPath() {
  if (!g_ffmpegResolved) {
    g_ffmpeg = locate("ffmpeg");
    g_ffmpegResolved = true;
  }
  return g_ffmpeg;
}

QString ffprobePath() {
  if (!g_ffprobeResolved) {
    g_ffprobe = locate("ffprobe");
    g_ffprobeResolved = true;
  }
  return g_ffprobe;
}

bool isAvailable() { return !ffmpegPath().isEmpty(); }

QString installHint() {
#ifdef Q_OS_WIN
  return QStringLiteral(
      "FFmpeg was not found. Install it, then restart the app:\n"
      "  - MSYS2/UCRT64:  pacman -S mingw-w64-ucrt-x86_64-ffmpeg\n"
      "  - winget:        winget install Gyan.FFmpeg\n"
      "  - or place ffmpeg.exe and ffprobe.exe in the app's bin\\ folder.");
#elif defined(Q_OS_MACOS)
  return QStringLiteral(
      "FFmpeg was not found. Install it, then restart the app:\n"
      "  - Homebrew:  brew install ffmpeg");
#else
  return QStringLiteral(
      "FFmpeg was not found. Install it, then restart the app:\n"
      "  - Debian/Ubuntu:  sudo apt install ffmpeg\n"
      "  - Fedora:         sudo dnf install ffmpeg");
#endif
}

} // namespace FfmpegLocator
