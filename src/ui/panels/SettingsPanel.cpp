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

    auto *mr = new QHBoxLayout();
    m_modelCombo = new QComboBox(card);
    m_modelCombo->setMinimumWidth(240);
    connect(m_modelCombo, &QComboBox::currentIndexChanged, [this] {
      QString path = m_modelCombo->currentData().toString();
      m_modelTierLabel->setText(
          path.isEmpty() ? ""
                         : "Tier: " + TranscriptionEngine::modelTier(path));
      emit settingsChanged();
    });
    mr->addWidget(new QLabel("AI Model:", card), 0);
    m_modelCombo->setToolTip(
        "Select the local AI whisper model to process audio transcription.");
    mr->addWidget(m_modelCombo, 1);

    auto *refreshBtn = new QPushButton("Refresh", card);
    refreshBtn->setObjectName("secondaryBtn");
    refreshBtn->setToolTip("Refresh model list");
    connect(refreshBtn, &QPushButton::clicked, this,
            &SettingsPanel::refreshModels);
    mr->addWidget(refreshBtn);

    auto *browseBtn = new QPushButton("Browse", card);
    browseBtn->setObjectName("secondaryBtn");
    connect(browseBtn, &QPushButton::clicked, this,
            &SettingsPanel::browseModels);
    mr->addWidget(browseBtn);
    cl->addLayout(mr);

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
    dlHint->setStyleSheet("color:#404040; font-size:7.5pt;");
    dlHint->setWordWrap(true);
    cl->addWidget(dlHint);

    m_modelTierLabel = new QLabel("", card);
    m_modelTierLabel->setStyleSheet("color:#505050; font-size:7.5pt;");
    cl->addWidget(m_modelTierLabel);

    auto *fr = new QHBoxLayout();
    fr->addWidget(new QLabel("Generation Mode:", card));
    fr->addStretch();
    m_fastModeToggle = new ToggleSwitch(card);
    m_fastModeToggle->setToolTip(
        "Toggles fast processing. Disabled means highest accuracy.");
    fr->addWidget(m_fastModeToggle);
    cl->addLayout(fr);

    auto *hint = new QLabel("Fast = tiny/base model, Greedy search. Accurate = "
                            "medium/large, Beam search.",
                            card);
    hint->setStyleSheet("color:#444444; font-size:7.5pt;");
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
    auto *fr1 = new QHBoxLayout();
    fr1->addWidget(new QLabel("Font:", card));
    fr1->addWidget(m_fontCombo);
    cl->addLayout(fr1);

    m_fontSizeSpin = new QSpinBox(card);
    m_fontSizeSpin->setRange(8, 120);
    m_fontSizeSpin->setValue(10);
    auto *fr2 = new QHBoxLayout();
    fr2->addWidget(new QLabel("Size:", card));
    fr2->addWidget(m_fontSizeSpin);
    cl->addLayout(fr2);

    m_colorBtn = new QPushButton("Pick Color", card);
    m_colorBtn->setStyleSheet(QString(
        "background-color: #FFFFFF; color: black; border: 1px solid #ccc;"));
    connect(m_colorBtn, &QPushButton::clicked, this, &SettingsPanel::pickColor);
    auto *fr3 = new QHBoxLayout();
    fr3->addWidget(new QLabel("Color:", card));
    fr3->addWidget(m_colorBtn);
    cl->addLayout(fr3);

    m_alignCombo = new QComboBox(card);
    m_alignCombo->addItems({"Bottom Center", "Bottom Left", "Bottom Right",
                            "Top Center", "Top Left", "Top Right", "Center",
                            "Center Left", "Center Right"});
    auto *fr4 = new QHBoxLayout();
    fr4->addWidget(new QLabel("Position:", card));
    fr4->addWidget(m_alignCombo);
    cl->addLayout(fr4);

    // Add Outline Thickness Option
    m_outlineSpin = new QSpinBox(card);
    m_outlineSpin->setRange(0, 10);
    m_outlineSpin->setValue(1);
    m_outlineSpin->setToolTip(
        "Thickness of the text border shadow/outline in pixels.");
    auto *fr5 = new QHBoxLayout();
    fr5->addWidget(new QLabel("Outline Thickness:", card));
    fr5->addWidget(m_outlineSpin);
    cl->addLayout(fr5);

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

    auto *or2 = new QHBoxLayout();
    m_outputDirEdit = new QLineEdit(card);
    m_outputDirEdit->setPlaceholderText("Select output directory...");
    m_outputDirEdit->setText(
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    or2->addWidget(new QLabel("Output Dir:", card));
    or2->addWidget(m_outputDirEdit, 1);
    auto *obtn = new QPushButton("Browse", card);
    obtn->setObjectName("secondaryBtn");
    connect(obtn, &QPushButton::clicked, this, &SettingsPanel::browseOutputDir);
    or2->addWidget(obtn);
    cl->addLayout(or2);

    auto *er = new QHBoxLayout();
    er->addWidget(new QLabel("Embed Subs:", card));
    er->addStretch();
    m_embedToggle = new ToggleSwitch(card);
    m_embedToggle->setChecked(true);
    er->addWidget(m_embedToggle);
    cl->addLayout(er);

    auto *pr = new QHBoxLayout();
    pr->addWidget(new QLabel("Parallel Pipelines:", card));
    pr->addStretch();
    m_parallelSpin = new QSpinBox(card);
    m_parallelSpin->setToolTip("Number of videos to process concurrently. "
                               "Increasing this requires more RAM.");
    m_parallelSpin->setRange(1, QThread::idealThreadCount());
    m_parallelSpin->setValue(1);
    m_parallelSpin->setFixedWidth(52);
    pr->addWidget(m_parallelSpin);
    cl->addLayout(pr);

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

    auto *tr2 = new QHBoxLayout();
    tr2->addWidget(new QLabel("Enable:", card));
    tr2->addStretch();
    m_translateToggle = new ToggleSwitch(card);
    tr2->addWidget(m_translateToggle);
    cl->addLayout(tr2);

    auto *slr = new QHBoxLayout();
    slr->addWidget(new QLabel("Source Lang:", card));
    slr->addStretch();
    m_srcLangCombo = new QComboBox(card);
    m_srcLangCombo->setMinimumWidth(160);
    for (const QString &lang : TranslationEngine::supportedLanguages())
      m_srcLangCombo->addItem(lang);
    slr->addWidget(m_srcLangCombo);
    cl->addLayout(slr);

    auto *tlr = new QHBoxLayout();
    tlr->addWidget(new QLabel("Target Lang:", card));
    tlr->addStretch();
    m_tgtLangCombo = new QComboBox(card);
    m_tgtLangCombo->setMinimumWidth(160);
    for (const QString &lang : TranslationEngine::supportedLanguages())
      m_tgtLangCombo->addItem(lang);
    m_tgtLangCombo->setCurrentText("English");
    tlr->addWidget(m_tgtLangCombo);
    cl->addLayout(tlr);

    auto *er2 = new QHBoxLayout();
    er2->addWidget(new QLabel("API Endpoint:", card));
    m_endpointEdit = new QLineEdit(card);
    m_endpointEdit->setPlaceholderText("http://localhost:5000");
    m_endpointEdit->setText("http://localhost:5000");
    er2->addWidget(m_endpointEdit, 1);
    cl->addLayout(er2);

    auto *hint = new QLabel(
        "Requires a running LibreTranslate server for offline use.", card);
    hint->setStyleSheet("color:#444444; font-size:7.5pt;");
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
  return m_tgtLangCombo->currentData().toString();
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
