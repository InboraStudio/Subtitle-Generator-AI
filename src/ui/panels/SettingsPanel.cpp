#include "SettingsPanel.h"
#include "../../core/TranscriptionEngine.h"
#include "../../core/TranslationEngine.h"
#include "../Theme.h"
#include "../widgets/ModelDownloadDialog.h"
#include <QColorDialog>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QScrollBar>
#include <QVBoxLayout>

namespace {
// Alignment combo index -> ASS numpad alignment.
const int kAssAlign[] = {2, 1, 3, 8, 7, 9, 5, 4, 6};
const char *kDefaultFont = "Arial";
const int kDefaultFontSize = 16;
const int kDefaultOutline = 1;
} // namespace

SettingsPanel::SettingsPanel(QWidget *parent) : QWidget(parent) {
  auto *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  auto *root = new QWidget(scroll);
  auto *vl = new QVBoxLayout(root);
  vl->setContentsMargins(20, 18, 20, 24);
  vl->setSpacing(16);

  QLabel *saved = nullptr;
  vl->addLayout(Theme::pageHeader(root, "Settings", &saved));
  saved->setText("Changes are saved automatically");

  // ---- Transcription model -------------------------------------------------
  {
    QVBoxLayout *cl = nullptr;
    QHBoxLayout *actions = nullptr;
    auto *card = Theme::card(root, "TRANSCRIPTION MODEL", &cl, &actions);

    auto *mgrBtn = new QPushButton("Model Manager", card);
    mgrBtn->setObjectName("ghostBtn");
    mgrBtn->setToolTip("Download Whisper models sized for this PC (Ctrl+M)");
    connect(mgrBtn, &QPushButton::clicked, this,
            &SettingsPanel::openModelManager);
    actions->addWidget(mgrBtn);

    m_modelCombo = new QComboBox(card);
    m_modelCombo->setToolTip(
        "The Whisper model used to transcribe audio. Larger models are more "
        "accurate but slower.");
    connect(m_modelCombo, &QComboBox::currentIndexChanged, this, [this] {
      updateModelStatus();
      onAnyChange();
    });

    auto *refreshBtn = new QPushButton("Refresh", card);
    refreshBtn->setObjectName("secondaryBtn");
    refreshBtn->setToolTip("Rescan the models folder");
    connect(refreshBtn, &QPushButton::clicked, this,
            &SettingsPanel::refreshModels);

    auto *browseBtn = new QPushButton("Browse", card);
    browseBtn->setObjectName("secondaryBtn");
    browseBtn->setToolTip("Use a model file from anywhere on disk");
    connect(browseBtn, &QPushButton::clicked, this,
            &SettingsPanel::browseModels);

    auto *modelWrap = new QWidget(card);
    auto *modelHl = new QHBoxLayout(modelWrap);
    modelHl->setContentsMargins(0, 0, 0, 0);
    modelHl->setSpacing(8);
    modelHl->addWidget(m_modelCombo, 1);
    modelHl->addWidget(refreshBtn);
    modelHl->addWidget(browseBtn);
    cl->addLayout(Theme::formRow(card, "AI Model", modelWrap, true));

    m_modelStatus = new QLabel(card);
    auto *statusRow = new QHBoxLayout();
    statusRow->addSpacing(152);
    statusRow->addWidget(m_modelStatus);
    statusRow->addStretch();
    cl->addLayout(statusRow);

    m_fastModeToggle = new ToggleSwitch(card);
    cl->addLayout(Theme::formRow(
        card, "Fast Mode", m_fastModeToggle, false,
        "On: greedy decoding, much faster. Off: beam search, most accurate."));

    auto *hint = new QLabel("Fast mode uses greedy decoding. Leave it off for "
                            "the best accuracy (beam search).",
                            card);
    Theme::setRole(hint, "hint");
    hint->setWordWrap(true);
    hint->setContentsMargins(152, 0, 0, 0);
    cl->addWidget(hint);

    vl->addWidget(card);
  }

  // ---- Subtitle appearance -------------------------------------------------
  {
    QVBoxLayout *cl = nullptr;
    QHBoxLayout *actions = nullptr;
    auto *card = Theme::card(root, "SUBTITLE APPEARANCE", &cl, &actions);

    auto *resetBtn = new QPushButton("Reset", card);
    resetBtn->setObjectName("ghostBtn");
    resetBtn->setToolTip("Restore the default subtitle style");
    connect(resetBtn, &QPushButton::clicked, this,
            &SettingsPanel::resetSubtitleStyle);
    actions->addWidget(resetBtn);

    auto *split = new QHBoxLayout();
    split->setSpacing(20);
    auto *form = new QVBoxLayout();
    form->setSpacing(12);

    m_fontCombo = new QFontComboBox(card);
    m_fontCombo->setCurrentFont(QFont(kDefaultFont));
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this,
            &SettingsPanel::onAnyChange);
    form->addLayout(Theme::formRow(card, "Font", m_fontCombo, true));

    m_fontSizeSpin = new QSpinBox(card);
    m_fontSizeSpin->setRange(8, 120);
    m_fontSizeSpin->setValue(kDefaultFontSize);
    m_fontSizeSpin->setFixedWidth(100);
    connect(m_fontSizeSpin, &QSpinBox::valueChanged, this,
            &SettingsPanel::onAnyChange);
    form->addLayout(Theme::formRow(card, "Size", m_fontSizeSpin, false,
                                   "Font size on a 288-line canvas, as FFmpeg "
                                   "renders it. 16-22 suits most videos."));

    m_colorBtn = new QPushButton(card);
    m_colorBtn->setObjectName("colorSwatch");
    m_colorBtn->setFixedWidth(100);
    m_colorBtn->setCursor(Qt::PointingHandCursor);
    connect(m_colorBtn, &QPushButton::clicked, this, &SettingsPanel::pickColor);
    form->addLayout(Theme::formRow(card, "Color", m_colorBtn, false));

    m_outlineSpin = new QSpinBox(card);
    m_outlineSpin->setRange(0, 10);
    m_outlineSpin->setValue(kDefaultOutline);
    m_outlineSpin->setSuffix(" px");
    m_outlineSpin->setFixedWidth(100);
    connect(m_outlineSpin, &QSpinBox::valueChanged, this,
            &SettingsPanel::onAnyChange);
    form->addLayout(Theme::formRow(card, "Outline", m_outlineSpin, false,
                                   "Thickness of the black text outline."));

    m_alignCombo = new QComboBox(card);
    m_alignCombo->addItems({"Bottom Center", "Bottom Left", "Bottom Right",
                            "Top Center", "Top Left", "Top Right", "Center",
                            "Center Left", "Center Right"});
    connect(m_alignCombo, &QComboBox::currentIndexChanged, this,
            &SettingsPanel::onAnyChange);
    form->addLayout(Theme::formRow(card, "Position", m_alignCombo, true));
    form->addStretch();

    split->addLayout(form, 1);

    auto *previewCol = new QVBoxLayout();
    previewCol->setSpacing(6);
    m_preview = new SubtitlePreview(card);
    m_preview->setMinimumWidth(280);
    previewCol->addWidget(m_preview, 1);
    auto *pHint = new QLabel("Preview of burned-in subtitles", card);
    Theme::setRole(pHint, "hint");
    pHint->setAlignment(Qt::AlignCenter);
    previewCol->addWidget(pHint);
    split->addLayout(previewCol, 1);

    cl->addLayout(split);
    vl->addWidget(card);
  }

  // ---- Export --------------------------------------------------------------
  {
    QVBoxLayout *cl = nullptr;
    auto *card = Theme::card(root, "EXPORT", &cl);

    m_outputDirEdit = new QLineEdit(card);
    m_outputDirEdit->setPlaceholderText("Select output directory...");
    m_outputDirEdit->setClearButtonEnabled(true);
    connect(m_outputDirEdit, &QLineEdit::textChanged, this, [this] {
      updateOutputStatus();
      onAnyChange();
    });
    auto *obtn = new QPushButton("Browse", card);
    obtn->setObjectName("secondaryBtn");
    connect(obtn, &QPushButton::clicked, this, &SettingsPanel::browseOutputDir);
    auto *openBtn = new QPushButton("Open", card);
    openBtn->setObjectName("secondaryBtn");
    openBtn->setToolTip("Open the output folder");
    connect(openBtn, &QPushButton::clicked, this, [this] {
      QDir().mkpath(outputDirectory());
      QDesktopServices::openUrl(QUrl::fromLocalFile(outputDirectory()));
    });

    auto *outWrap = new QWidget(card);
    auto *outHl = new QHBoxLayout(outWrap);
    outHl->setContentsMargins(0, 0, 0, 0);
    outHl->setSpacing(8);
    outHl->addWidget(m_outputDirEdit, 1);
    outHl->addWidget(obtn);
    outHl->addWidget(openBtn);
    cl->addLayout(Theme::formRow(card, "Output Folder", outWrap, true));

    m_outputStatus = new QLabel(card);
    m_outputStatus->setTextFormat(Qt::PlainText);
    m_outputStatus->setContentsMargins(152, 0, 0, 0);
    cl->addWidget(m_outputStatus);

    m_embedToggle = new ToggleSwitch(card);
    m_embedToggle->setChecked(true);
    connect(m_embedToggle, &ToggleSwitch::toggled, this,
            &SettingsPanel::onAnyChange);
    cl->addLayout(Theme::formRow(
        card, "Burn Into Video", m_embedToggle, false,
        "Also export a copy of each video with the subtitles burned in "
        "([name]_subbed.mp4). The .srt file is always written."));

    m_parallelSpin = new QSpinBox(card);
    m_parallelSpin->setRange(1, qMax(1, QThread::idealThreadCount()));
    m_parallelSpin->setValue(1);
    m_parallelSpin->setFixedWidth(100);
    connect(m_parallelSpin, &QSpinBox::valueChanged, this,
            &SettingsPanel::onAnyChange);
    cl->addLayout(Theme::formRow(
        card, "Parallel Jobs", m_parallelSpin, false,
        "Videos processed at the same time. Each job loads its own model, "
        "so RAM use multiplies."));

    vl->addWidget(card);
  }

  // ---- Translation ---------------------------------------------------------
  {
    QVBoxLayout *cl = nullptr;
    auto *card = Theme::card(root, "LANGUAGE & TRANSLATION", &cl);

    m_srcLangCombo = new QComboBox(card);
    for (const QString &lang : TranslationEngine::supportedLanguages())
      m_srcLangCombo->addItem(lang);
    connect(m_srcLangCombo, &QComboBox::currentIndexChanged, this,
            &SettingsPanel::onAnyChange);
    cl->addLayout(Theme::formRow(card, "Spoken Language", m_srcLangCombo, true,
                                 "Language spoken in the videos. Auto Detect "
                                 "works well with multilingual models."));

    m_translateToggle = new ToggleSwitch(card);
    connect(m_translateToggle, &ToggleSwitch::toggled, this, [this](bool on) {
      for (QWidget *w : m_translationFields)
        w->setEnabled(on);
      onAnyChange();
    });
    cl->addLayout(Theme::formRow(card, "Translate", m_translateToggle, false));

    m_tgtLangCombo = new QComboBox(card);
    for (const QString &lang : TranslationEngine::supportedLanguages())
      if (lang != "Auto Detect")
        m_tgtLangCombo->addItem(lang);
    m_tgtLangCombo->setCurrentText("English");
    connect(m_tgtLangCombo, &QComboBox::currentIndexChanged, this,
            &SettingsPanel::onAnyChange);
    cl->addLayout(Theme::formRow(card, "Translate To", m_tgtLangCombo, true));

    m_endpointEdit = new QLineEdit(card);
    m_endpointEdit->setPlaceholderText("http://localhost:5000");
    m_endpointEdit->setText("http://localhost:5000");
    connect(m_endpointEdit, &QLineEdit::textChanged, this,
            &SettingsPanel::onAnyChange);
    cl->addLayout(Theme::formRow(card, "Server URL", m_endpointEdit, true,
                                 "LibreTranslate server used for "
                                 "translation."));

    auto *hint = new QLabel("Translation needs a running LibreTranslate server "
                            "(self-hosted, works offline).",
                            card);
    Theme::setRole(hint, "hint");
    hint->setWordWrap(true);
    hint->setContentsMargins(152, 0, 0, 0);
    cl->addWidget(hint);

    m_translationFields = {m_tgtLangCombo, m_endpointEdit};
    for (QWidget *w : m_translationFields)
      w->setEnabled(false);

    vl->addWidget(card);
  }

  vl->addStretch();
  scroll->setWidget(root);

  auto *outerVl = new QVBoxLayout(this);
  outerVl->setContentsMargins(0, 0, 0, 0);
  outerVl->addWidget(scroll);

  // Scrolling the page must not silently change whatever combo or spin box
  // happens to pass under the cursor; those only take the wheel once focused.
  m_scrollBar = scroll->verticalScrollBar();
  const auto inputs = root->findChildren<QWidget *>();
  for (QWidget *w : inputs) {
    if (qobject_cast<QComboBox *>(w) || qobject_cast<QAbstractSpinBox *>(w)) {
      w->setFocusPolicy(Qt::StrongFocus);
      w->installEventFilter(this);
    }
  }

  loadSettings();
}

bool SettingsPanel::eventFilter(QObject *obj, QEvent *e) {
  if (e->type() == QEvent::Wheel) {
    auto *w = qobject_cast<QWidget *>(obj);
    if (w && !w->hasFocus()) {
      QCoreApplication::sendEvent(m_scrollBar, e);
      return true;
    }
  }
  return QWidget::eventFilter(obj, e);
}

SettingsPanel::~SettingsPanel() { saveSettings(); }

void SettingsPanel::loadSettings() {
  m_loading = true;
  QSettings s;
  refreshModels();
  const QString model = s.value("model/path").toString();
  if (!model.isEmpty() && QFileInfo::exists(model))
    setModelPath(model);
  m_fastModeToggle->setChecked(s.value("model/fast", false).toBool());

  m_fontCombo->setCurrentFont(QFont(s.value("style/font", kDefaultFont).toString()));
  m_fontSizeSpin->setValue(s.value("style/size", kDefaultFontSize).toInt());
  applyColor(QColor("#" + s.value("style/color", "FFFFFF").toString()));
  m_outlineSpin->setValue(s.value("style/outline", kDefaultOutline).toInt());
  m_alignCombo->setCurrentIndex(s.value("style/align", 0).toInt());

  m_outputDirEdit->setText(QDir::toNativeSeparators(
      s.value("export/dir", QStandardPaths::writableLocation(
                                QStandardPaths::DesktopLocation))
          .toString()));
  m_embedToggle->setChecked(s.value("export/embed", true).toBool());
  m_parallelSpin->setValue(s.value("export/parallel", 1).toInt());

  m_srcLangCombo->setCurrentText(
      s.value("lang/source", "Auto Detect").toString());
  m_tgtLangCombo->setCurrentText(s.value("lang/target", "English").toString());
  m_translateToggle->setChecked(s.value("lang/translate", false).toBool());
  m_endpointEdit->setText(
      s.value("lang/endpoint", "http://localhost:5000").toString());
  m_loading = false;

  updateModelStatus();
  updateOutputStatus();
  updatePreview();
}

void SettingsPanel::saveSettings() const {
  QSettings s;
  s.setValue("model/path", selectedModelPath());
  s.setValue("model/fast", fastModeEnabled());
  s.setValue("style/font", subtitleFontName());
  s.setValue("style/size", subtitleFontSize());
  s.setValue("style/color", m_currentColorHex);
  s.setValue("style/outline", subtitleOutlineThickness());
  s.setValue("style/align", m_alignCombo->currentIndex());
  s.setValue("export/dir", outputDirectory());
  s.setValue("export/embed", embedSubtitlesEnabled());
  s.setValue("export/parallel", parallelJobCount());
  s.setValue("lang/source", m_srcLangCombo->currentText());
  s.setValue("lang/target", m_tgtLangCombo->currentText());
  s.setValue("lang/translate", translationEnabled());
  s.setValue("lang/endpoint", translationEndpoint());
}

void SettingsPanel::onAnyChange() {
  if (m_loading)
    return;
  updatePreview();
  saveSettings();
  emit settingsChanged();
}

void SettingsPanel::resetSubtitleStyle() {
  m_loading = true;
  m_fontCombo->setCurrentFont(QFont(kDefaultFont));
  m_fontSizeSpin->setValue(kDefaultFontSize);
  applyColor(Qt::white);
  m_outlineSpin->setValue(kDefaultOutline);
  m_alignCombo->setCurrentIndex(0);
  m_loading = false;
  onAnyChange();
}

void SettingsPanel::updatePreview() {
  m_preview->setFontFamily(subtitleFontName());
  m_preview->setFontSize(subtitleFontSize());
  m_preview->setTextColor(QColor("#" + m_currentColorHex));
  m_preview->setOutline(subtitleOutlineThickness());
  m_preview->setAlignment(subtitleAlignment());
}

void SettingsPanel::updateModelStatus() {
  const QString path = selectedModelPath();
  if (path.isEmpty()) {
    m_modelStatus->setText("No model installed - open the Model Manager to "
                           "download one.");
    Theme::setRole(m_modelStatus, "badgeError");
    return;
  }
  QFileInfo fi(path);
  if (!fi.exists()) {
    m_modelStatus->setText("Model file not found: " +
                           QDir::toNativeSeparators(path));
    Theme::setRole(m_modelStatus, "badgeError");
    return;
  }
  m_modelStatus->setText(QString("Ready  |  %1  |  %2  |  %3")
                             .arg(TranscriptionEngine::modelTier(path))
                             .arg(Theme::formatBytes(fi.size()))
                             .arg(fi.fileName().contains(".en")
                                      ? "English only"
                                      : "Multilingual"));
  Theme::setRole(m_modelStatus, "badgeOk");
}

void SettingsPanel::updateOutputStatus() {
  const QString dir = outputDirectory();
  QFileInfo fi(dir);
  bool bad = false;
  if (dir.trimmed().isEmpty()) {
    m_outputStatus->setText("Choose where .srt files will be saved.");
    bad = true;
  } else if (fi.exists() && !fi.isDir()) {
    m_outputStatus->setText("This path is a file, not a folder.");
    bad = true;
  } else if (!fi.exists()) {
    m_outputStatus->setText("Folder will be created when processing starts.");
  } else {
    m_outputStatus->setText("Subtitles are saved here as [video name].srt");
  }
  Theme::setRole(m_outputStatus, bad ? "error" : "hint");
  m_outputDirEdit->setProperty("invalid", bad);
  Theme::repolish(m_outputDirEdit);
}

void SettingsPanel::browseOutputDir() {
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory", m_outputDirEdit->text());
  if (!dir.isEmpty())
    m_outputDirEdit->setText(QDir::toNativeSeparators(dir));
}

void SettingsPanel::browseModels() {
  QString path = QFileDialog::getOpenFileName(
      this, "Select Model File", TranscriptionEngine::userModelsDir(),
      "Model Files (*.bin *.gguf);;All Files (*)");
  if (!path.isEmpty())
    setModelPath(path);
}

void SettingsPanel::refreshModels() {
  const QString current = selectedModelPath();
  const bool wasLoading = m_loading;
  m_loading = true;
  m_modelCombo->clear();
  QStringList models = TranscriptionEngine::discoverModels();
  if (models.isEmpty()) {
    m_modelCombo->addItem("No models found - open the Model Manager", "");
  } else {
    for (const QString &m : models)
      m_modelCombo->addItem(QFileInfo(m).fileName(), m);
  }
  if (!current.isEmpty()) {
    const int idx = m_modelCombo->findData(current);
    if (idx >= 0)
      m_modelCombo->setCurrentIndex(idx);
  }
  m_loading = wasLoading;
  updateModelStatus();
  onAnyChange();
}

void SettingsPanel::openModelManager() {
  ModelDownloadDialog dlg(TranscriptionEngine::userModelsDir(), this);
  dlg.exec();
  refreshModels();
  if (!dlg.lastDownloadedPath().isEmpty())
    setModelPath(dlg.lastDownloadedPath());
}

void SettingsPanel::setModelPath(const QString &path) {
  int idx = m_modelCombo->findData(path);
  if (idx < 0) {
    // Drop the "No models found" placeholder once a real model exists.
    if (m_modelCombo->count() == 1 &&
        m_modelCombo->itemData(0).toString().isEmpty())
      m_modelCombo->clear();
    m_modelCombo->insertItem(0, QFileInfo(path).fileName(), path);
    idx = 0;
  }
  m_modelCombo->setCurrentIndex(idx);
  updateModelStatus();
}

void SettingsPanel::applyColor(const QColor &color) {
  if (!color.isValid())
    return;
  m_currentColorHex = color.name().remove('#').toUpper();
  const QString textCol = color.lightness() < 128 ? "#FFFFFF" : "#0E0E0E";
  m_colorBtn->setText("#" + m_currentColorHex);
  m_colorBtn->setStyleSheet(
      QString("QPushButton#colorSwatch { background-color:%1; color:%2; }")
          .arg(color.name(), textCol));
}

void SettingsPanel::pickColor() {
  QColor color = QColorDialog::getColor(QColor("#" + m_currentColorHex), this,
                                        "Select Subtitle Color");
  if (color.isValid()) {
    applyColor(color);
    onAnyChange();
  }
}

QString SettingsPanel::summary() const {
  const QString model = selectedModelPath();
  return QString("%1  |  %2%3")
      .arg(model.isEmpty() ? QString("No model") : QFileInfo(model).fileName())
      .arg(m_srcLangCombo->currentText())
      .arg(translationEnabled() ? " -> " + m_tgtLangCombo->currentText()
                                : QString());
}

QString SettingsPanel::selectedModelPath() const {
  return m_modelCombo->currentData().toString();
}
QString SettingsPanel::outputDirectory() const {
  return QDir::fromNativeSeparators(m_outputDirEdit->text().trimmed());
}
QString SettingsPanel::sourceLanguage() const {
  return TranslationEngine::languageCode(m_srcLangCombo->currentText());
}
QString SettingsPanel::targetLanguage() const {
  // The combo stores display names (no per-item data), so resolve the code the
  // same way sourceLanguage() does.
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
  return m_endpointEdit->text().trimmed();
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
  const int i = m_alignCombo->currentIndex();
  return (i >= 0 && i < 9) ? kAssAlign[i] : 2;
}
