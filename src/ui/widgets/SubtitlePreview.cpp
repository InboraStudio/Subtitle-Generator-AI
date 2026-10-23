#include "SubtitlePreview.h"
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

SubtitlePreview::SubtitlePreview(QWidget *parent) : QWidget(parent) {
  setMinimumSize(240, 135);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void SubtitlePreview::setFontFamily(const QString &family) {
  m_family = family;
  update();
}
void SubtitlePreview::setFontSize(int size) {
  m_size = size;
  update();
}
void SubtitlePreview::setTextColor(const QColor &color) {
  m_color = color;
  update();
}
void SubtitlePreview::setOutline(int px) {
  m_outline = px;
  update();
}
void SubtitlePreview::setAlignment(int a) {
  m_align = a;
  update();
}

QSize SubtitlePreview::sizeHint() const { return QSize(480, 270); }

void SubtitlePreview::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  // Letterbox a 16:9 frame inside whatever space the layout gives us.
  QRectF frame = rect().adjusted(0, 0, -1, -1);
  const double fh = qMin(frame.height(), frame.width() * 9.0 / 16.0);
  const double fw = fh * 16.0 / 9.0;
  frame = QRectF(frame.center().x() - fw / 2, frame.center().y() - fh / 2,
                 fw, fh);

  QPainterPath clip;
  clip.addRoundedRect(frame, 6, 6);
  p.setClipPath(clip);

  // A dim "scene" so light and dark text colours are both judged fairly.
  QLinearGradient sky(frame.topLeft(), frame.bottomLeft());
  sky.setColorAt(0.0, QColor("#2B3A4A"));
  sky.setColorAt(0.6, QColor("#4A4038"));
  sky.setColorAt(1.0, QColor("#1E1A16"));
  p.fillRect(frame, sky);
  QPainterPath hills;
  hills.moveTo(frame.left(), frame.bottom());
  hills.cubicTo(frame.left() + fw * 0.25, frame.top() + fh * 0.55,
                frame.left() + fw * 0.55, frame.top() + fh * 0.85,
                frame.right(), frame.top() + fh * 0.6);
  hills.lineTo(frame.right(), frame.bottom());
  p.fillPath(hills, QColor(20, 18, 16, 180));

  // libass scales Fontsize against a 288-line canvas for SRT input.
  const double scale = fh / 288.0;
  QFont f(m_family);
  f.setPixelSize(qMax(6, int(m_size * scale)));
  f.setBold(true);

  const QString text = "The quick brown fox jumps over the lazy dog";
  const double margin = 10 * scale + 6;
  QRectF area = frame.adjusted(margin, margin, -margin, -margin);

  int hAlign = Qt::AlignHCenter;
  const int col = (m_align - 1) % 3;
  if (col == 0)
    hAlign = Qt::AlignLeft;
  else if (col == 2)
    hAlign = Qt::AlignRight;
  int vAlign = Qt::AlignBottom;
  if (m_align >= 7)
    vAlign = Qt::AlignTop;
  else if (m_align >= 4)
    vAlign = Qt::AlignVCenter;

  QFontMetricsF fm(f);
  // Wrap by hand and build the glyph outline line by line, so the stroke
  // follows the wrapped text exactly.
  QPainterPath path;
  const QStringList words = text.split(' ');
  QStringList lines;
  QString cur;
  for (const QString &w : words) {
    QString trial = cur.isEmpty() ? w : cur + ' ' + w;
    if (fm.horizontalAdvance(trial) > area.width() && !cur.isEmpty()) {
      lines << cur;
      cur = w;
    } else {
      cur = trial;
    }
  }
  if (!cur.isEmpty())
    lines << cur;

  const double blockH = lines.size() * fm.lineSpacing();
  double top = area.bottom() - blockH;
  if (vAlign == Qt::AlignTop)
    top = area.top();
  else if (vAlign == Qt::AlignVCenter)
    top = area.center().y() - blockH / 2;
  double y = top + fm.ascent();
  for (const QString &line : lines) {
    const double lw = fm.horizontalAdvance(line);
    double x = area.left();
    if (hAlign == Qt::AlignHCenter)
      x = area.center().x() - lw / 2;
    else if (hAlign == Qt::AlignRight)
      x = area.right() - lw;
    path.addText(x, y, f, line);
    y += fm.lineSpacing();
  }

  if (m_outline > 0) {
    QPen pen(Qt::black, m_outline * scale * 2.0, Qt::SolidLine, Qt::RoundCap,
             Qt::RoundJoin);
    p.strokePath(path, pen);
  }
  p.fillPath(path, m_color);

  p.setClipping(false);
  p.setPen(QPen(QColor("#2A2A2A"), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(frame, 6, 6);
}
