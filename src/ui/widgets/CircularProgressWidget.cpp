#include "CircularProgressWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

CircularProgressWidget::CircularProgressWidget(QWidget *parent)
    : QWidget(parent) {
  setFixedSize(80, 80);
}

int CircularProgressWidget::value() const { return m_value; }

void CircularProgressWidget::setValue(int val) {
  m_value = qBound(0, val, 100);
  update();
}

void CircularProgressWidget::setLabel(const QString &label) {
  m_label = label;
  update();
}

void CircularProgressWidget::setAccentColor(const QColor &color) {
  m_accent = color;
  update();
}

void CircularProgressWidget::setThickness(int px) {
  m_thickness = px;
  update();
}

QSize CircularProgressWidget::sizeHint() const { return QSize(80, 80); }

void CircularProgressWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  int margin = m_thickness / 2 + 2;
  QRectF rect(margin, margin, width() - margin * 2, height() - margin * 2);

  QPen trackPen(QColor("#252525"), m_thickness, Qt::SolidLine, Qt::RoundCap);
  p.setPen(trackPen);
  p.drawArc(rect, 0, 360 * 16);

  if (m_value > 0) {
    QPen arcPen(m_accent, m_thickness, Qt::SolidLine, Qt::RoundCap);
    p.setPen(arcPen);
    int spanAngle = static_cast<int>(-360.0 * m_value / 100.0 * 16.0);
    p.drawArc(rect, 90 * 16, spanAngle);
  }

  p.setPen(QColor("#E0E0E0"));
  QFont font = p.font();
  font.setPointSize(11);
  font.setBold(true);
  p.setFont(font);
  p.drawText(rect, Qt::AlignCenter, QString::number(m_value) + "%");

  if (!m_label.isEmpty()) {
    QFont lf = p.font();
    lf.setPointSize(7);
    lf.setBold(false);
    p.setFont(lf);
    p.setPen(QColor("#505050"));
    QRectF labelRect(0, height() - 14, width(), 14);
    p.drawText(labelRect, Qt::AlignCenter, m_label);
  }
}
