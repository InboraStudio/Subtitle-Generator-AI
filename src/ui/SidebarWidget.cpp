#include "SidebarWidget.h"
#include "Theme.h"
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {
// Most nav icons come from the PNG set; "info" is drawn in code.
QIcon navIcon(const QString &name, const QColor &color) {
  return name == "info" ? Theme::glyph(name, color) : Theme::icon(name, color);
}
} // namespace

SidebarWidget::SidebarWidget(QWidget *parent) : QFrame(parent) {
  setObjectName("sidebar");
  setFixedWidth(184);

  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(0, 16, 0, 12);
  vl->setSpacing(2);

  {
    auto *brand = new QVBoxLayout();
    brand->setContentsMargins(16, 0, 16, 18);
    brand->setSpacing(1);
    auto *logo = new QLabel("SG-AI", this);
    logo->setObjectName("brandMark");
    brand->addWidget(logo);
    auto *sub = new QLabel("by Inbora Studio", this);
    sub->setObjectName("brandSub");
    brand->addWidget(sub);
    vl->addLayout(brand);
  }

  auto addSection = [&](const QString &text) {
    auto *l = new QLabel(text, this);
    l->setObjectName("navSection");
    vl->addSpacing(6);
    vl->addWidget(l);
    vl->addSpacing(2);
  };

  addSection("WORKFLOW");
  const struct {
    const char *label;
    const char *icon;
    const char *tip;
  } tabs[] = {
      {"Input", "input", "Add videos to the queue (Ctrl+1)"},
      {"Settings", "settings", "Model, subtitle style and export (Ctrl+2)"},
      {"Progress", "progress", "Live processing status (Ctrl+3)"},
  };
  for (int i = 0; i < 3; ++i) {
    QLabel *badge = nullptr;
    auto *btn = makeNavButton(tabs[i].label, tabs[i].icon, &badge);
    btn->setToolTip(tabs[i].tip);
    connect(btn, &QPushButton::clicked, this, [this, i] {
      setActiveTab(i);
      emit tabSelected(i);
    });
    vl->addWidget(btn);
    m_tabs.append({btn, badge, tabs[i].icon});
  }

  addSection("TOOLS");
  {
    auto *btn = makeNavButton("Model Manager", "save", nullptr);
    btn->setToolTip("Download Whisper models (Ctrl+M)");
    connect(btn, &QPushButton::clicked, this,
            &SidebarWidget::modelManagerRequested);
    vl->addWidget(btn);
  }
  {
    const int index = m_tabs.size();
    auto *btn = makeNavButton("About", "info", nullptr);
    btn->setToolTip("Inbora Studio, Discord and GitHub");
    connect(btn, &QPushButton::clicked, this, [this, index] {
      setActiveTab(index);
      emit tabSelected(index);
    });
    vl->addWidget(btn);
    m_tabs.append({btn, nullptr, "info"});
  }

  vl->addStretch();

  auto *ver = new QLabel(
      "Version " + QCoreApplication::applicationVersion(), this);
  ver->setObjectName("brandSub");
  ver->setContentsMargins(16, 0, 16, 0);
  vl->addWidget(ver);

  setActiveTab(0);
}

QPushButton *SidebarWidget::makeNavButton(const QString &label,
                                          const QString &iconName,
                                          QLabel **badge) {
  auto *btn = new QPushButton(label, this);
  btn->setObjectName("sidebarBtn");
  btn->setIcon(navIcon(iconName, Theme::Color::TextMuted));
  btn->setIconSize(QSize(16, 16));
  btn->setCursor(Qt::PointingHandCursor);
  btn->setFocusPolicy(Qt::NoFocus);

  if (badge) {
    auto *hl = new QHBoxLayout(btn);
    hl->setContentsMargins(0, 0, 12, 0);
    hl->addStretch();
    *badge = new QLabel(btn);
    (*badge)->setObjectName("navBadge");
    (*badge)->setAlignment(Qt::AlignCenter);
    (*badge)->setAttribute(Qt::WA_TransparentForMouseEvents);
    (*badge)->hide();
    hl->addWidget(*badge);
  }
  return btn;
}

void SidebarWidget::setActiveTab(int index) {
  m_activeIndex = index;
  for (int i = 0; i < m_tabs.size(); ++i) {
    const bool active = i == index;
    QPushButton *b = m_tabs[i].button;
    b->setProperty("active", active ? "true" : "false");
    b->setIcon(navIcon(m_tabs[i].iconName, active ? Theme::Color::Accent
                                                  : Theme::Color::TextMuted));
    Theme::repolish(b);
  }
}

void SidebarWidget::setBadge(int index, const QString &text, bool accent) {
  if (index < 0 || index >= m_tabs.size())
    return;
  QLabel *b = m_tabs[index].badge;
  if (!b)
    return;
  b->setText(text);
  b->setVisible(!text.isEmpty());
  Theme::setRole(b, accent ? "accent" : "");
}
