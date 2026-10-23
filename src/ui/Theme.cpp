#include "Theme.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QVBoxLayout>

namespace Theme {

namespace {
// Draws a chevron and saves it as a PNG: QSS can only reference image files,
// and the CSS border-triangle trick renders as a solid block in Qt.
QString writeChevron(const QString &dir, const QString &name, bool up,
                     const QColor &color) {
  QPixmap pm(20, 12);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(color, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  if (up)
    p.drawPolyline(QPolygonF({{2, 10}, {10, 2.5}, {18, 10}}));
  else
    p.drawPolyline(QPolygonF({{2, 2}, {10, 9.5}, {18, 2}}));
  p.end();
  const QString path = dir + "/" + name + ".png";
  pm.save(path);
  return path;
}
} // namespace

void apply(QApplication &app) {
  QPalette pal;
  pal.setColor(QPalette::Window, Color::Window);
  pal.setColor(QPalette::WindowText, Color::Text);
  pal.setColor(QPalette::Base, QColor("#1A1A1A"));
  pal.setColor(QPalette::AlternateBase, Color::SurfaceRaised);
  pal.setColor(QPalette::Text, Color::Text);
  pal.setColor(QPalette::Button, QColor("#242424"));
  pal.setColor(QPalette::ButtonText, Color::Text);
  pal.setColor(QPalette::Highlight, Color::Accent);
  pal.setColor(QPalette::HighlightedText, QColor("#0E0E0E"));
  pal.setColor(QPalette::ToolTipBase, QColor("#222222"));
  pal.setColor(QPalette::ToolTipText, Color::Text);
  pal.setColor(QPalette::PlaceholderText, Color::TextFaint);
  pal.setColor(QPalette::Link, Color::Accent);
  pal.setColor(QPalette::Disabled, QPalette::Text, QColor("#404040"));
  pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#404040"));
  pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#404040"));
  app.setPalette(pal);

  QFile f(":/styles/dark.qss");
  if (!f.open(QFile::ReadOnly | QFile::Text))
    return;
  QString qss = QString::fromUtf8(f.readAll());
#ifndef Q_OS_WIN
  // Segoe UI only ships with Windows; elsewhere Qt spends ~100 ms building
  // font aliases to replace it, so use the platform UI font instead.
  qss.replace(QStringLiteral("\"Segoe UI\""),
              QStringLiteral("\"%1\"").arg(
                  QFontDatabase::systemFont(QFontDatabase::GeneralFont)
                      .family()));
#endif
  const QString dir =
      QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
      "/SubtitleGeneratorAI/theme";
  QDir().mkpath(dir);
  const QColor idle("#7A7A7A");
  qss.replace("@CHEVRON_DOWN@", writeChevron(dir, "down", false, idle));
  qss.replace("@CHEVRON_UP@", writeChevron(dir, "up", true, idle));
  qss.replace("@CHEVRON_DOWN_ACCENT@",
              writeChevron(dir, "down-accent", false, Color::Accent));
  qss.replace("@CHEVRON_UP_ACCENT@",
              writeChevron(dir, "up-accent", true, Color::Accent));
  {
    QPixmap pm(28, 28);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#0E0E0E"), 3.2, Qt::SolidLine, Qt::RoundCap,
                  Qt::RoundJoin));
    p.drawPolyline(QPolygonF({{6, 14.5}, {11.5, 20}, {22, 8}}));
    p.end();
    pm.save(dir + "/check.png");
    qss.replace("@CHECK@", dir + "/check.png");
  }
  app.setStyleSheet(qss);
}

void repolish(QWidget *w) {
  if (!w)
    return;
  w->style()->unpolish(w);
  w->style()->polish(w);
  // Padding/font can change with the new style, so the layout must re-measure.
  w->updateGeometry();
  w->update();
}

void setRole(QWidget *w, const QString &role) {
  if (!w || w->property("role").toString() == role)
    return;
  w->setProperty("role", role);
  repolish(w);
}

QIcon icon(const QString &name, const QColor &color) {
  static QHash<QString, QIcon> cache;
  const QString key = name + color.name();
  auto it = cache.constFind(key);
  if (it != cache.constEnd())
    return *it;

  QPixmap src(QString(":/icons/%1.png").arg(name));
  if (src.isNull())
    return QIcon();
  QPixmap out(src.size());
  out.fill(Qt::transparent);
  QPainter p(&out);
  p.drawPixmap(0, 0, src);
  p.setCompositionMode(QPainter::CompositionMode_SourceIn);
  p.fillRect(out.rect(), color);
  p.end();

  QIcon ic(out);
  cache.insert(key, ic);
  return ic;
}

QIcon glyph(const QString &kind, const QColor &color) {
  // Painted at 2x for crisp lines on high-DPI screens.
  QPixmap pm(20, 20);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing, kind == "close" || kind == "info");
  p.setPen(QPen(color, 1.6));
  if (kind == "info") {
    p.drawEllipse(QRectF(2, 2, 16, 16));
    p.setPen(QPen(color, 2.0, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(10, 9), QPointF(10, 14.5));
    p.drawPoint(QPointF(10, 6));
  } else if (kind == "min") {
    p.drawLine(QPointF(3, 10.5), QPointF(17, 10.5));
  } else if (kind == "max") {
    p.drawRect(QRectF(3.5, 3.5, 13, 13));
  } else if (kind == "restore") {
    p.drawRect(QRectF(3.5, 6.5, 10, 10));
    p.drawPolyline(QPolygonF({{6.5, 6.5}, {6.5, 3.5}, {16.5, 3.5},
                              {16.5, 13.5}, {13.5, 13.5}}));
  } else {
    p.drawLine(QPointF(4, 4), QPointF(16, 16));
    p.drawLine(QPointF(16, 4), QPointF(4, 16));
  }
  p.end();
  return QIcon(pm);
}

QHBoxLayout *pageHeader(QWidget *parent, const QString &title,
                        QLabel **detail) {
  auto *row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(10);

  auto *t = new QLabel(title, parent);
  setRole(t, "pageTitle");
  row->addWidget(t);
  row->addStretch();

  if (detail) {
    *detail = new QLabel(parent);
    setRole(*detail, "faint");
    row->addWidget(*detail);
  }
  return row;
}

QFrame *card(QWidget *parent, const QString &title, QVBoxLayout **body,
             QHBoxLayout **headerActions) {
  auto *c = new QFrame(parent);
  c->setObjectName("card");
  auto *outer = new QVBoxLayout(c);
  outer->setContentsMargins(1, 1, 1, 1);
  outer->setSpacing(0);

  auto *hdr = new QFrame(c);
  hdr->setObjectName("cardHeader");
  hdr->setFixedHeight(34);
  auto *hl = new QHBoxLayout(hdr);
  hl->setContentsMargins(14, 0, 10, 0);
  hl->setSpacing(6);
  auto *t = new QLabel(title, hdr);
  t->setObjectName("cardTitle");
  hl->addWidget(t);
  hl->addStretch();
  outer->addWidget(hdr);
  if (headerActions)
    *headerActions = hl;

  auto *content = new QVBoxLayout();
  content->setContentsMargins(16, 14, 16, 16);
  content->setSpacing(12);
  outer->addLayout(content);
  if (body)
    *body = content;
  return c;
}

QHBoxLayout *formRow(QWidget *parent, const QString &label, QWidget *control,
                     bool expand, const QString &tooltip) {
  auto *row = new QHBoxLayout();
  row->setSpacing(12);
  auto *lbl = new QLabel(label, parent);
  lbl->setFixedWidth(140);
  setRole(lbl, "fieldLabel");
  if (!tooltip.isEmpty()) {
    lbl->setToolTip(tooltip);
    control->setToolTip(tooltip);
  }
  row->addWidget(lbl, 0);
  if (expand) {
    row->addWidget(control, 1);
  } else {
    row->addWidget(control, 0);
    row->addStretch();
  }
  return row;
}

QString formatBytes(qint64 bytes) {
  if (bytes >= 1024LL * 1024 * 1024)
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 1);
  if (bytes >= 1024 * 1024)
    return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 0);
  if (bytes >= 1024)
    return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 0);
  return QString("%1 B").arg(bytes);
}

QString formatDuration(int s) {
  if (s < 60)
    return QString("%1s").arg(s);
  if (s < 3600)
    return QString("%1m %2s").arg(s / 60).arg(s % 60);
  return QString("%1h %2m").arg(s / 3600).arg((s % 3600) / 60);
}

} // namespace Theme
