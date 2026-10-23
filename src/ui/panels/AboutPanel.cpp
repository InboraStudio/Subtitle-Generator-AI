#include "AboutPanel.h"
#include "../Theme.h"
#include <QCoreApplication>
#include <QDate>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

AboutPanel::AboutPanel(QWidget *parent) : QWidget(parent) {
  auto *outer = new QVBoxLayout(this);
  outer->setContentsMargins(20, 18, 20, 18);
  outer->addStretch(1);

  auto *card = new QFrame(this);
  card->setObjectName("aboutCard");
  card->setFixedWidth(520);
  auto *vl = new QVBoxLayout(card);
  vl->setContentsMargins(40, 36, 40, 30);
  vl->setSpacing(0);

  auto centered = [&](QWidget *w, int spaceAfter) {
    vl->addWidget(w, 0, Qt::AlignHCenter);
    vl->addSpacing(spaceAfter);
  };

  auto *logo = new QLabel(card);
  logo->setPixmap(QIcon(":/icons/app.png").pixmap(76, 76));
  centered(logo, 16);

  auto *name = new QLabel("Subtitle Generator AI", card);
  name->setObjectName("aboutTitle");
  centered(name, 4);

  auto *ver = new QLabel(
      QString("Version %1").arg(QCoreApplication::applicationVersion()), card);
  Theme::setRole(ver, "faint");
  centered(ver, 14);

  auto *tagline = new QLabel("Accurate subtitles for any video, generated "
                             "entirely on your PC with Whisper AI.",
                             card);
  tagline->setObjectName("aboutTagline");
  tagline->setWordWrap(true);
  tagline->setAlignment(Qt::AlignCenter);
  vl->addWidget(tagline);
  vl->addSpacing(16);

  {
    auto *chips = new QHBoxLayout();
    chips->setSpacing(8);
    chips->addStretch();
    for (const char *t : {"Runs offline", "Whisper AI", "Batch processing",
                          "99 languages"}) {
      auto *c = new QLabel(t, card);
      c->setObjectName("chip");
      chips->addWidget(c);
    }
    chips->addStretch();
    vl->addLayout(chips);
  }
  vl->addSpacing(26);

  auto *div = new QFrame(card);
  div->setObjectName("divider");
  vl->addWidget(div);
  vl->addSpacing(22);

  auto *madeBy = new QLabel("MADE BY", card);
  Theme::setRole(madeBy, "statLabel");
  centered(madeBy, 6);

  auto *studio = new QLabel("Inbora Studio", card);
  studio->setObjectName("aboutStudio");
  centered(studio, 2);

  auto *author = new QLabel("Dr Chamyoung", card);
  Theme::setRole(author, "muted");
  centered(author, 22);

  {
    auto *row = new QHBoxLayout();
    row->setSpacing(10);
    row->addStretch();

    auto *discord = new QPushButton("Join our Discord", card);
    discord->setObjectName("primaryBtn");
    discord->setCursor(Qt::PointingHandCursor);
    discord->setToolTip(kDiscordUrl);
    connect(discord, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(QUrl(kDiscordUrl)); });
    row->addWidget(discord);

    auto *github = new QPushButton("GitHub", card);
    github->setObjectName("secondaryBtn");
    github->setCursor(Qt::PointingHandCursor);
    github->setToolTip(kGithubUrl);
    connect(github, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(QUrl(kGithubUrl)); });
    row->addWidget(github);

    row->addStretch();
    vl->addLayout(row);
  }
  vl->addSpacing(12);

  {
    auto *row = new QHBoxLayout();
    row->setSpacing(4);
    row->addStretch();
    auto addLink = [&](const QString &text, const QString &url) {
      auto *b = new QPushButton(text, card);
      b->setObjectName("ghostBtn");
      b->setCursor(Qt::PointingHandCursor);
      b->setToolTip(url);
      connect(b, &QPushButton::clicked, this,
              [url] { QDesktopServices::openUrl(QUrl(url)); });
      row->addWidget(b);
    };
    addLink("Check for updates", QString(kGithubUrl) + "/releases/latest");
    addLink("Report a bug", QString(kGithubUrl) + "/issues");
    row->addStretch();
    vl->addLayout(row);
  }

  auto *cardRow = new QHBoxLayout();
  cardRow->addStretch();
  cardRow->addWidget(card);
  cardRow->addStretch();
  outer->addLayout(cardRow);
  outer->addSpacing(14);

  auto *copy = new QLabel(
      QString("© %1 Inbora Studio").arg(QDate::currentDate().year()), this);
  Theme::setRole(copy, "faint");
  outer->addWidget(copy, 0, Qt::AlignHCenter);
  outer->addStretch(1);
}
