#pragma once
#include "../widgets/SubtitlePreview.h"
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
  ~SettingsPanel() override;

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

  // Short "model - language - output" line for the status bar / input page.
  QString summary() const;

public slots:
  void openModelManager();
  void saveSettings() const;
  void resetSubtitleStyle();

signals:
  void settingsChanged();

private slots:
  void browseOutputDir();
  void browseModels();
  void refreshModels();
  void pickColor();
  void onAnyChange();

private:
  QComboBox *m_modelCombo;
  QLabel *m_modelStatus;
  QLineEdit *m_outputDirEdit;
  QLabel *m_outputStatus;
  QComboBox *m_srcLangCombo;
  QComboBox *m_tgtLangCombo;
  ToggleSwitch *m_translateToggle;
  ToggleSwitch *m_embedToggle;
  ToggleSwitch *m_fastModeToggle;
  QSpinBox *m_parallelSpin;
  QLineEdit *m_endpointEdit;
  QList<QWidget *> m_translationFields;

  // Subtitle styling widgets
  QFontComboBox *m_fontCombo;
  QSpinBox *m_fontSizeSpin;
  QSpinBox *m_outlineSpin;
  QComboBox *m_alignCombo;
  QPushButton *m_colorBtn;
  SubtitlePreview *m_preview;
  QString m_currentColorHex = "FFFFFF";

  bool m_loading = false;
  QWidget *m_scrollBar = nullptr;

protected:
  bool eventFilter(QObject *obj, QEvent *e) override;

private:

  void loadSettings();
  void applyColor(const QColor &color);
  void updateModelStatus();
  void updateOutputStatus();
  void updatePreview();
};
