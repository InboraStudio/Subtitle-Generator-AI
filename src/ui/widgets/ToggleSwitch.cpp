#include "ToggleSwitch.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>


ToggleSwitch::ToggleSwitch(QWidget *parent) : QWidget(parent) {
  setCursor(Qt::PointingHandCursor);
  setFixedSize(38, 20);

  m_anim = new QPropertyAnimation(this, "offset", this);
  m_anim->setDuration(150);
  m_anim->setEasingCurve(QEasingCurve::InOutCubic);

  m_offset = 2.0f;
}

bool ToggleSwitch::isChecked() const { return m_checked; }

void ToggleSwitch::setChecked(bool checked) {
  if (m_checked == checked)
    return;
  m_checked = checked;
  m_anim->stop();
  m_anim->setStartValue(m_offset);
  m_anim->setEndValue(checked ? 20.0f : 2.0f);
  m_anim->start();
  update();
  emit toggled(m_checked);
}

QSize ToggleSwitch::sizeHint() const { return QSize(38, 20); }
QSize ToggleSwitch::minimumSizeHint() const { return sizeHint(); }

void ToggleSwitch::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QColor track = m_checked ? QColor("#E8872A") : QColor("#2E2E2E");
  p.setBrush(track);
  p.setPen(Qt::NoPen);
  p.drawRoundedRect(0, 3, width(), height() - 6, 7, 7);

  p.setBrush(QColor("#E0E0E0"));
  p.drawEllipse(QRectF(m_offset, 2, 16, 16));
}

void ToggleSwitch::mousePressEvent(QMouseEvent *) { setChecked(!m_checked); }
