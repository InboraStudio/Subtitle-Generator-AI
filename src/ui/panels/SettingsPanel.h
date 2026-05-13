#pragma once
#include "../widgets/ToggleSwitch.h"
#include <QComboBox>
#include <QFontComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

class SettingsPanel : public QWidget {
  Q_OBJECT
public:
  explicit SettingsPanel(QWidget *parent = nullptr);

  QString selectedModelPath() const;
  QString outputDirectory() const;
  QString sourceLanguage() const;
  QString targetLanguage() const;
  bool translationEnabled() const;
  bool embedSubtitlesEnabled() const;
  bool fastModeEnabled() const;
  int parallelJobCount() const;
  QString translationEndpoint() const;
  void setModelPath(const QString &path);

  // Subtitle customizer
  QString subtitleFontName() const;
  int subtitleFontSize() const;
  QString subtitleFontColor() const;
  int subtitleOutlineThickness() const;
  int subtitleAlignment() const;

signals:
  void settingsChanged();

private slots:
  void browseOutputDir();
  void browseModels();
  void refreshModels();
  void openModelManager();
  void pickColor();

private:
  QComboBox *m_modelCombo;
  QLineEdit *m_outputDirEdit;
  QComboBox *m_srcLangCombo;
  QComboBox *m_tgtLangCombo;
  ToggleSwitch *m_translateToggle;
  ToggleSwitch *m_embedToggle;
  ToggleSwitch *m_fastModeToggle;
  QSpinBox *m_parallelSpin;
  QLineEdit *m_endpointEdit;
  QLabel *m_modelTierLabel;

  // Subtitle styling widgets
  QFontComboBox *m_fontCombo;
  QSpinBox *m_fontSizeSpin;
  QSpinBox *m_outlineSpin;
  QComboBox *m_alignCombo;
  QPushButton *m_colorBtn;
  QString m_currentColorHex = "FFFFFF";

  QWidget *makeCard(QWidget *content, const QString &title);
  QWidget *makeRow(QWidget *label, QWidget *control);
};
