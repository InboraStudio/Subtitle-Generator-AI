#include "SettingsPanel.h"
#include "../../core/TranscriptionEngine.h"
#include "../../core/TranslationEngine.h"
#include "../widgets/ModelDownloadDialog.h"
#include <QColorDialog>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QStandardPaths>
#include <QThread>
#include <QVBoxLayout>

static QFrame *makeDivider(QWidget *parent) {
  auto *d = new QFrame(parent);
  d->setObjectName("divider");
  return d;
}

// Builds a consistent "label : control" form row so every settings row shares
// the same label column width and controls align vertically down the card.
// When expand is true the control fills the remaining width (combos, line
// edits); otherwise it keeps its natural size and is pushed to the right
// (toggles, spin boxes).
static QHBoxLayout *formRow(QWidget *parent, const QString &labelText,
                            QWidget *control, bool expand) {
  auto *row = new QHBoxLayout();
  row->setSpacing(12);
  auto *lbl = new QLabel(labelText, parent);
  lbl->setMinimumWidth(130);
  lbl->setStyleSheet("color:#B0B0B0; font-size:9pt;");
  row->addWidget(lbl, 0);
  if (expand) {
    row->addWidget(control, 1);
  } else {
    row->addStretch();
    row->addWidget(control, 0);
  }
  return row;
}

SettingsPanel::SettingsPanel(QWidget *parent) : QWidget(parent) {
  auto *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  auto *root = new QWidget(scroll);
  auto *vl = new QVBoxLayout(root);
  vl->setContentsMargins(16, 16, 16, 24);
  vl->setSpacing(16);

  auto *title = new QLabel("Global Settings & Configuration", root);
  title->setStyleSheet("color:#FFFFFF; font-size:12pt; font-weight:bold; "
                       "letter-spacing: 0.5px; margin-bottom: 5px;");
  vl->addWidget(title);

  {
    auto *card = new QFrame(root);
    card->setObjectName("settingsCard");
    card->setStyleSheet("QFrame#settingsCard{background:#161616;border:1px solid "
                        "#2D2D2D;border-radius:10px;}");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(18, 16, 18, 16);
    cl->setSpacing(12);

    auto *ct = new QLabel("Transcription Model Preferences", card);
    ct->setStyleSheet("color:#A0A0A0; font-size:8pt; letter-spacing:1px; "
                      "font-weight:bold; text-transform:uppercase;");
    cl->addWidget(ct);
    cl->addWidget(makeDivider(card));

    m_modelCombo = new QComboBox(card);
    connect(m_modelCombo, &QComboBox::currentIndexChanged, [this] {
      QString path = m_modelCombo->currentData().toString();
      m_modelTierLabel->setText(
          path.isEmpty() ? ""
                         : "Tier: " + TranscriptionEngine::modelTier(path));
      emit settingsChanged();
    });
    m_modelCombo->setToolTip(
        "Select the local AI whisper model to process audio transcription.");

    auto *refreshBtn = new QPushButton("Refresh", card);
    refreshBtn->setObjectName("secondaryBtn");
    refreshBtn->setToolTip("Refresh model list");
    connect(refreshBtn, &QPushButton::clicked, this,
            &SettingsPanel::refreshModels);

    auto *browseBtn = new QPushButton("Browse", card);
    browseBtn->setObjectName("secondaryBtn");
    connect(browseBtn, &QPushButton::clicked, this,
            &SettingsPanel::browseModels);

    {
      auto *modelWrap = new QWidget(card);
      auto *modelHl = new QHBoxLayout(modelWrap);
      modelHl->setContentsMargins(0, 0, 0, 0);
      modelHl->setSpacing(8);
      modelHl->addWidget(m_modelCombo, 1);
      modelHl->addWidget(refreshBtn, 0);
      modelHl->addWidget(browseBtn, 0);
      cl->addLayout(formRow(card, "AI Model:", modelWrap, true));
    }

    auto *getModelsRow = new QHBoxLayout();
    auto *getModelsBtn = new QPushButton("Get Models", card);
    getModelsBtn->setObjectName("primaryBtn");
    getModelsBtn->setToolTip(
        "Open the Model Manager to download whisper models");
    getModelsBtn->setStyleSheet("QPushButton#primaryBtn { font-size:8.5pt; "
                                "padding:5px 14px; min-height:26px; }");
    connect(getModelsBtn, &QPushButton::clicked, this,
            &SettingsPanel::openModelManager);
    getModelsRow->addWidget(getModelsBtn);
    getModelsRow->addStretch();
    cl->addLayout(getModelsRow);

    auto *dlHint = new QLabel(
        "Model Manager detects your system RAM and recommends the best model.",
        card);
    dlHint->setStyleSheet("color:#6E6E6E; font-size:8pt;");
    dlHint->setWordWrap(true);
    cl->addWidget(dlHint);

    m_modelTierLabel = new QLabel("", card);
    m_modelTierLabel->setStyleSheet("color:#8A8A8A; font-size:8pt;");
    cl->addWidget(m_modelTierLabel);

    m_fastModeToggle = new ToggleSwitch(card);
    m_fastModeToggle->setToolTip(
        "Toggles fast processing. Disabled means highest accuracy.");
    cl->addLayout(formRow(card, "Generation Mode:", m_fastModeToggle, false));

    auto *hint = new QLabel("Fast = tiny/base model, Greedy search. Accurate = "
                            "medium/large, Beam search.",
                            card);
    hint->setStyleSheet("color:#6E6E6E; font-size:8pt;");
    hint->setWordWrap(true);
    cl->addWidget(hint);

    vl->addWidget(card);
  }

  {
    auto *card = new QFrame(root);
    card->setObjectName("settingsCard");
    card->setStyleSheet("QFrame#settingsCard{background:#161616;border:1px solid "
                        "#2D2D2D;border-radius:10px;}");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(18, 16, 18, 16);
    cl->setSpacing(12);

    auto *ct = new QLabel("Subtitle Appearance", card);
    ct->setStyleSheet("color:#A0A0A0; font-size:8pt; letter-spacing:1px; "
                      "font-weight:bold; text-transform:uppercase;");
    cl->addWidget(ct);
    cl->addWidget(makeDivider(card));

    m_fontCombo = new QFontComboBox(card);
    m_fontCombo->setCurrentFont(QFont("Arial"));
    cl->addLayout(formRow(card, "Font:", m_fontCombo, true));

    m_fontSizeSpin = new QSpinBox(card);
    m_fontSizeSpin->setRange(8, 120);
    m_fontSizeSpin->setValue(10);
    m_fontSizeSpin->setFixedWidth(90);
    cl->addLayout(formRow(card, "Size:", m_fontSizeSpin, false));

    m_colorBtn = new QPushButton("Pick Color", card);
    m_colorBtn->setFixedWidth(120);
    m_colorBtn->setMinimumHeight(28);
    m_colorBtn->setStyleSheet(
        "background-color:#FFFFFF; color:#000000; border:1px solid #3A3A3A; "
        "border-radius:6px; font-weight:600;");
    connect(m_colorBtn, &QPushButton::clicked, this, &SettingsPanel::pickColor);
    cl->addLayout(formRow(card, "Color:", m_colorBtn, false));

    m_alignCombo = new QComboBox(card);
    m_alignCombo->addItems({"Bottom Center", "Bottom Left", "Bottom Right",
                            "Top Center", "Top Left", "Top Right", "Center",
                            "Center Left", "Center Right"});
    cl->addLayout(formRow(card, "Position:", m_alignCombo, true));

    m_outlineSpin = new QSpinBox(card);
    m_outlineSpin->setRange(0, 10);
    m_outlineSpin->setValue(1);
    m_outlineSpin->setFixedWidth(90);
    m_outlineSpin->setToolTip(
        "Thickness of the text border shadow/outline in pixels.");
    cl->addLayout(formRow(card, "Outline Thickness:", m_outlineSpin, false));

    vl->addWidget(card);
  }

  {
    auto *card = new QFrame(root);
    card->setObjectName("settingsCard");
    card->setStyleSheet("QFrame#settingsCard{background:#161616;border:1px solid "
                        "#2D2D2D;border-radius:10px;}");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(18, 16, 18, 16);
    cl->setSpacing(12);

    auto *ct = new QLabel("Export Profile", card);
    ct->setStyleSheet("color:#A0A0A0; font-size:8pt; letter-spacing:1px; "
                      "font-weight:bold; text-transform:uppercase;");
    cl->addWidget(ct);
    cl->addWidget(makeDivider(card));

    m_outputDirEdit = new QLineEdit(card);
    m_outputDirEdit->setPlaceholderText("Select output directory...");
    m_outputDirEdit->setText(
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    auto *obtn = new QPushButton("Browse", card);
    obtn->setObjectName("secondaryBtn");
    connect(obtn, &QPushButton::clicked, this, &SettingsPanel::browseOutputDir);
    {
      auto *outWrap = new QWidget(card);
      auto *outHl = new QHBoxLayout(outWrap);
      outHl->setContentsMargins(0, 0, 0, 0);
      outHl->setSpacing(8);
      outHl->addWidget(m_outputDirEdit, 1);
      outHl->addWidget(obtn, 0);
      cl->addLayout(formRow(card, "Output Dir:", outWrap, true));
    }

    m_embedToggle = new ToggleSwitch(card);
    m_embedToggle->setChecked(true);
    cl->addLayout(formRow(card, "Embed Subs:", m_embedToggle, false));

    m_parallelSpin = new QSpinBox(card);
    m_parallelSpin->setToolTip("Number of videos to process concurrently. "
                               "Increasing this requires more RAM.");
    m_parallelSpin->setRange(1, QThread::idealThreadCount());
    m_parallelSpin->setValue(1);
    m_parallelSpin->setFixedWidth(90);
    cl->addLayout(formRow(card, "Parallel Pipelines:", m_parallelSpin, false));

    vl->addWidget(card);
  }

  {
    auto *card = new QFrame(root);
    card->setObjectName("settingsCard");
    card->setStyleSheet("QFrame#settingsCard{background:#161616;border:1px solid "
                        "#2D2D2D;border-radius:10px;}");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(18, 16, 18, 16);
    cl->setSpacing(12);

    auto *ct = new QLabel("Translation Rules", card);
    ct->setStyleSheet("color:#A0A0A0; font-size:8pt; letter-spacing:1px; "
                      "font-weight:bold; text-transform:uppercase;");
    cl->addWidget(ct);
    cl->addWidget(makeDivider(card));

    m_translateToggle = new ToggleSwitch(card);
    cl->addLayout(formRow(card, "Enable:", m_translateToggle, false));

    m_srcLangCombo = new QComboBox(card);
    for (const QString &lang : TranslationEngine::supportedLanguages())
      m_srcLangCombo->addItem(lang);
    cl->addLayout(formRow(card, "Source Lang:", m_srcLangCombo, true));

    m_tgtLangCombo = new QComboBox(card);
    for (const QString &lang : TranslationEngine::supportedLanguages())
      m_tgtLangCombo->addItem(lang);
    m_tgtLangCombo->setCurrentText("English");
    cl->addLayout(formRow(card, "Target Lang:", m_tgtLangCombo, true));

    m_endpointEdit = new QLineEdit(card);
    m_endpointEdit->setPlaceholderText("http://localhost:5000");
    m_endpointEdit->setText("http://localhost:5000");
    cl->addLayout(formRow(card, "API Endpoint:", m_endpointEdit, true));

    auto *hint = new QLabel(
        "Requires a running LibreTranslate server for offline use.", card);
    hint->setStyleSheet("color:#6E6E6E; font-size:8pt;");
    hint->setWordWrap(true);
    cl->addWidget(hint);

    vl->addWidget(card);
  }

  vl->addStretch();
  scroll->setWidget(root);

  auto *outerVl = new QVBoxLayout(this);
  outerVl->setContentsMargins(0, 0, 0, 0);
  outerVl->addWidget(scroll);

  refreshModels();
}

void SettingsPanel::browseOutputDir() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory", m_outputDirEdit->text());
  if (!dir.isEmpty())
    m_outputDirEdit->setText(dir);
}

void SettingsPanel::browseModels() {
  QString path = QFileDialog::getOpenFileName(
      this, "Select Model File",
      QCoreApplication::applicationDirPath() + "/models",
      "Model Files (*.bin *.gguf);;All Files (*)");
  if (!path.isEmpty()) {
    m_modelCombo->insertItem(0, QFileInfo(path).fileName(), path);
    m_modelCombo->setCurrentIndex(0);
  }
}

void SettingsPanel::refreshModels() {
  m_modelCombo->clear();
  QString modelsDir = QCoreApplication::applicationDirPath() + "/models";
  QStringList models = TranscriptionEngine::discoverModels(modelsDir);
  if (models.isEmpty()) {
    m_modelCombo->addItem("No models found - click 'Get Models'", "");
  } else {
    for (const QString &m : models)
      m_modelCombo->addItem(QFileInfo(m).fileName(), m);
  }
}

void SettingsPanel::openModelManager() {
  QString modelsDir = QCoreApplication::applicationDirPath() + "/models";
  ModelDownloadDialog dlg(modelsDir, this);
  dlg.exec();
  refreshModels();
  if (!dlg.lastDownloadedPath().isEmpty())
    setModelPath(dlg.lastDownloadedPath());
}

void SettingsPanel::setModelPath(const QString &path) {
  for (int i = 0; i < m_modelCombo->count(); ++i) {
    if (m_modelCombo->itemData(i).toString() == path) {
      m_modelCombo->setCurrentIndex(i);
      return;
    }
  }
  m_modelCombo->insertItem(0, QFileInfo(path).fileName(), path);
  m_modelCombo->setCurrentIndex(0);
}

void SettingsPanel::pickColor() {
  QColor color = QColorDialog::getColor(QColor("#" + m_currentColorHex), this,
                                        "Select Subtitle Color");
  if (color.isValid()) {
    m_currentColorHex = color.name().remove('#').toUpper();
    QString textCol = color.lightness() < 128 ? "white" : "black";
    m_colorBtn->setStyleSheet(
        QString("background-color: %1; color: %2; border: 1px solid #ccc;")
            .arg(color.name())
            .arg(textCol));
    emit settingsChanged();
  }
}

QString SettingsPanel::selectedModelPath() const {
  return m_modelCombo->currentData().toString();
}
QString SettingsPanel::outputDirectory() const {
  return m_outputDirEdit->text();
}
QString SettingsPanel::sourceLanguage() const {
  return TranslationEngine::languageCode(m_srcLangCombo->currentText());
}
QString SettingsPanel::targetLanguage() const {
  // The combo stores display names (no per-item data), so resolve the code the
  // same way sourceLanguage() does. Using currentData() here always returned an
  // empty string, which silently disabled translation downstream.
  return TranslationEngine::languageCode(m_tgtLangCombo->currentText());
}

bool SettingsPanel::translationEnabled() const {
  return m_translateToggle->isChecked();
}

bool SettingsPanel::embedSubtitlesEnabled() const {
  return m_embedToggle->isChecked();
}

bool SettingsPanel::fastModeEnabled() const {
  return m_fastModeToggle->isChecked();
}

int SettingsPanel::parallelJobCount() const { return m_parallelSpin->value(); }

QString SettingsPanel::translationEndpoint() const {
  return m_endpointEdit->text();
}

QString SettingsPanel::subtitleFontName() const {
  return m_fontCombo->currentFont().family();
}

int SettingsPanel::subtitleFontSize() const { return m_fontSizeSpin->value(); }

QString SettingsPanel::subtitleFontColor() const { return m_currentColorHex; }

int SettingsPanel::subtitleOutlineThickness() const {
  return m_outlineSpin->value();
}

int SettingsPanel::subtitleAlignment() const {
  switch (m_alignCombo->currentIndex()) {
  case 0:
    return 2; // Bottom Center
  case 1:
    return 1; // Bottom Left
  case 2:
    return 3; // Bottom Right
  case 3:
    return 8; // Top Center
  case 4:
    return 7; // Top Left
  case 5:
    return 9; // Top Right
  case 6:
    return 5; // Center
  case 7:
    return 4; // Center Left
  case 8:
    return 6; // Center Right
  default:
    return 2;
  }
}
