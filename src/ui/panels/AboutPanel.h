#pragma once
#include <QWidget>

// Credits and community links for Inbora Studio.
class AboutPanel : public QWidget {
  Q_OBJECT
public:
  explicit AboutPanel(QWidget *parent = nullptr);

  static constexpr const char *kDiscordUrl = "https://discord.gg/pVA8NZ5mru";
  static constexpr const char *kGithubUrl =
      "https://github.com/InboraStudio/Subtitle-Generator-AI";
};
