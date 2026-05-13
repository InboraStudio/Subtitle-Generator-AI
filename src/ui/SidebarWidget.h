#pragma once
#include <QList>
#include <QPushButton>
#include <QWidget>


class SidebarWidget : public QWidget {
  Q_OBJECT
public:
  explicit SidebarWidget(QWidget *parent = nullptr);

  void setActiveTab(int index);

signals:
  void tabSelected(int index);

private:
  QList<QPushButton *> m_tabs;
  int m_activeIndex = 0;

  void addTab(int index, const QString &label, const QString &iconPath);
};
