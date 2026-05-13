#include "SidebarWidget.h"
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

SidebarWidget::SidebarWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("sidebar");
  setFixedWidth(130);

  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(8, 12, 8, 12);
  vl->setSpacing(2);
  vl->setAlignment(Qt::AlignTop);

  auto *logo = new QLabel("SG-AI", this);
  logo->setStyleSheet("color:#E8872A; font-size:12pt; font-weight:700; "
                      "letter-spacing:1px; padding:4px 6px 16px 6px;");
  vl->addWidget(logo);

  addTab(0, "  Input", ":/icons/input.png");
  addTab(1, "  Settings", ":/icons/settings.png");
  addTab(2, "  Progress", ":/icons/progress.png");

  vl->addStretch();

  auto *ver = new QLabel("v2.0", this);
  ver->setObjectName("appVersion");
  ver->setStyleSheet("color:#303030; font-size:7.5pt; padding:4px 8px;");
  vl->addWidget(ver);

  setActiveTab(0);
}

void SidebarWidget::addTab(int index, const QString &label,
                           const QString &iconPath) {
  Q_UNUSED(iconPath)
  auto *btn = new QPushButton(label, this);
  btn->setObjectName("sidebarBtn");
  btn->setCheckable(false);
  btn->setFlat(true);
  connect(btn, &QPushButton::clicked, [this, index] {
    setActiveTab(index);
    emit tabSelected(index);
  });
  static_cast<QVBoxLayout *>(layout())->addWidget(btn);
  m_tabs.append(btn);
}

void SidebarWidget::setActiveTab(int index) {
  m_activeIndex = index;
  for (int i = 0; i < m_tabs.size(); ++i) {
    m_tabs[i]->setProperty("active", i == index ? "true" : "false");
    m_tabs[i]->style()->unpolish(m_tabs[i]);
    m_tabs[i]->style()->polish(m_tabs[i]);
  }
}
