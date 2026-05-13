#pragma once
#include <QWidget>

class CircularProgressWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int value READ value WRITE setValue)
public:
  explicit CircularProgressWidget(QWidget *parent = nullptr);

  int value() const;
  void setValue(int val);
  void setLabel(const QString &label);
  void setAccentColor(const QColor &color);
  void setThickness(int px);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  int m_value = 0;
  QString m_label;
  QColor m_accent = QColor("#E8872A");
  int m_thickness = 8;
};
