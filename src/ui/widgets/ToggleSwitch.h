#pragma once
#include <QPropertyAnimation>
#include <QWidget>


class ToggleSwitch : public QWidget {
  Q_OBJECT
  Q_PROPERTY(float offset READ offset WRITE setOffset)
public:
  explicit ToggleSwitch(QWidget *parent = nullptr);

  bool isChecked() const;
  void setChecked(bool checked);

  float offset() const { return m_offset; }
  void setOffset(float o) {
    m_offset = o;
    update();
  }

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

signals:
  void toggled(bool checked);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  bool m_checked = false;
  float m_offset = 0.0f;
  QPropertyAnimation *m_anim;
};
