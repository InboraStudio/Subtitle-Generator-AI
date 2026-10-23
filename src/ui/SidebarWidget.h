#pragma once
#include <QFrame>
#include <QLabel>
#include <QList>
#include <QPushButton>

class SidebarWidget : public QFrame {
  Q_OBJECT
public:
  explicit SidebarWidget(QWidget *parent = nullptr);

  void setActiveTab(int index);
  // Small counter on the right of a nav item; empty text hides it. Accent
  // badges draw in the brand colour (used while something is running).
  void setBadge(int index, const QString &text, bool accent = false);

signals:
  void tabSelected(int index);
  void modelManagerRequested();

private:
  struct NavItem {
    QPushButton *button;
    QLabel *badge;
    QString iconName;
  };
  QList<NavItem> m_tabs;
  int m_activeIndex = 0;

  QPushButton *makeNavButton(const QString &label, const QString &iconName,
                             QLabel **badge);
};
