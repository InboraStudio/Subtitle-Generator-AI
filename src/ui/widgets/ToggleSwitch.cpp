#include "ToggleSwitch.h"
#include "../Theme.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>


ToggleSwitch::ToggleSwitch(QWidget *parent) : QWidget(parent) {
  setCursor(Qt::PointingHandCursor);
  setFocusPolicy(Qt::StrongFocus);
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

  QColor track = m_checked ? Theme::Color::Accent : QColor("#2E2E2E");
  QColor knob = Theme::Color::TextStrong;
  if (!isEnabled()) {
    track = m_checked ? QColor("#4A3420") : QColor("#222222");
    knob = QColor("#505050");
  }
  p.setBrush(track);
  p.setPen(hasFocus() ? QPen(QColor("#6A4A26"), 1) : QPen(Qt::NoPen));
  p.drawRoundedRect(QRectF(0.5, 3.5, width() - 1, height() - 7), 6.5, 6.5);

  p.setPen(Qt::NoPen);
  p.setBrush(knob);
  p.drawEllipse(QRectF(m_offset, 2, 16, 16));
}

void ToggleSwitch::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton && isEnabled())
    setChecked(!m_checked);
}

void ToggleSwitch::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Return ||
      e->key() == Qt::Key_Enter)
    setChecked(!m_checked);
  else
    QWidget::keyPressEvent(e);
}
