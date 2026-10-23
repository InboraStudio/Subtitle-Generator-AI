#pragma once
#include <QColor>
#include <QWidget>

// Mock 16:9 video frame showing how burned-in subtitles will look with the
// current font, size, colour, outline and position. Sizes follow libass's
// default 288-line script resolution so the preview matches FFmpeg's output.
class SubtitlePreview : public QWidget {
  Q_OBJECT
public:
  explicit SubtitlePreview(QWidget *parent = nullptr);

  void setFontFamily(const QString &family);
  void setFontSize(int size);
  void setTextColor(const QColor &color);
  void setOutline(int px);
  // ASS numpad alignment: 1-3 bottom, 4-6 middle, 7-9 top.
  void setAlignment(int assAlignment);

  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override { return w * 9 / 16; }

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_family = "Arial";
  int m_size = 10;
  QColor m_color = Qt::white;
  int m_outline = 1;
  int m_align = 2;
};
