#include "JobListWidget.h"
#include "../Theme.h"
#include <QDesktopServices>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

static QString statusToString(JobStatus s) {
  switch (s) {
  case JobStatus::Queued:
    return "Queued";
  case JobStatus::Extracting:
    return "Extracting";
  case JobStatus::Transcribing:
    return "Transcribing";
  case JobStatus::Translating:
    return "Translating";
  case JobStatus::Writing:
    return "Writing SRT";
  case JobStatus::Embedding:
    return "Embedding";
  case JobStatus::Completed:
    return "Done";
  case JobStatus::Failed:
    return "Failed";
  case JobStatus::Cancelled:
    return "Cancelled";
  }
  return "Unknown";
}

static QString statusState(JobStatus s) {
  switch (s) {
  case JobStatus::Queued:
    return "queued";
  case JobStatus::Completed:
    return "ok";
  case JobStatus::Failed:
    return "error";
  case JobStatus::Cancelled:
    return "cancelled";
  default:
    return "running";
  }
}

JobListItem::JobListItem(const Job &job, QWidget *parent)
    : QWidget(parent), m_jobId(job.id),
      m_fullName(QFileInfo(job.inputPath).fileName()) {
  setFixedHeight(58);
  setToolTip(job.inputPath);

  auto *hl = new QHBoxLayout(this);
  hl->setContentsMargins(14, 8, 14, 8);
  hl->setSpacing(14);

  auto *left = new QVBoxLayout();
  left->setSpacing(5);

  auto *top = new QHBoxLayout();
  top->setSpacing(8);
  m_nameLabel = new QLabel(this);
  m_nameLabel->setObjectName("jobName");
  m_nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  top->addWidget(m_nameLabel, 1);
  m_stageLabel = new QLabel(this);
  Theme::setRole(m_stageLabel, "faint");
  top->addWidget(m_stageLabel);
  left->addLayout(top);

  m_bar = new QProgressBar(this);
  m_bar->setRange(0, 100);
  m_bar->setValue(0);
  m_bar->setTextVisible(false);
  m_bar->setProperty("thin", true);
  left->addWidget(m_bar);
  hl->addLayout(left, 1);

  m_percentLabel = new QLabel("0%", this);
  m_percentLabel->setFixedWidth(36);
  m_percentLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  Theme::setRole(m_percentLabel, "muted");
  hl->addWidget(m_percentLabel);

  m_pill = new QLabel(this);
  m_pill->setObjectName("pill");
  m_pill->setAlignment(Qt::AlignCenter);
  m_pill->setFixedWidth(90);
  hl->addWidget(m_pill);

  m_showBtn = new QPushButton("Show", this);
  m_showBtn->setObjectName("ghostBtn");
  m_showBtn->setToolTip("Open the folder containing the subtitle file");
  m_showBtn->setFixedWidth(52);
  m_showBtn->setEnabled(false);
  connect(m_showBtn, &QPushButton::clicked, this, [this] {
    if (!m_outputPath.isEmpty())
      QDesktopServices::openUrl(
          QUrl::fromLocalFile(QFileInfo(m_outputPath).absolutePath()));
  });
  hl->addWidget(m_showBtn);

  setStatus(JobStatus::Queued);
}

void JobListItem::elideName() {
  QFontMetrics fm(m_nameLabel->font());
  m_nameLabel->setText(
      fm.elidedText(m_fullName, Qt::ElideMiddle, qMax(40, m_nameLabel->width())));
}

void JobListItem::resizeEvent(QResizeEvent *e) {
  QWidget::resizeEvent(e);
  elideName();
}

void JobListItem::updateProgress(int percent, const QString &stage) {
  m_bar->setValue(percent);
  m_percentLabel->setText(QString("%1%").arg(percent));
  m_stageLabel->setText(stage);
  if (m_status == JobStatus::Queued)
    setStatus(JobStatus::Extracting);
  if (stage.startsWith("Transcrib"))
    m_pill->setText("Transcribing");
  else if (stage.startsWith("Translat"))
    m_pill->setText("Translating");
  else if (stage.startsWith("Writing"))
    m_pill->setText("Writing");
  else if (stage.startsWith("Embed"))
    m_pill->setText("Embedding");
}

void JobListItem::setStatus(JobStatus status) {
  m_status = status;
  m_pill->setText(statusToString(status));
  m_pill->setProperty("state", statusState(status));
  Theme::repolish(m_pill);

  QString barState;
  if (status == JobStatus::Completed) {
    m_bar->setValue(100);
    m_percentLabel->setText("100%");
    m_stageLabel->setText(QString());
    barState = "ok";
  } else if (status == JobStatus::Failed) {
    barState = "error";
  } else if (status == JobStatus::Cancelled) {
    m_stageLabel->setText(QString());
  }
  m_bar->setProperty("state", barState);
  Theme::repolish(m_bar);
}

void JobListItem::setError(const QString &error) {
  // Collapse newlines so the one-line stage label stays tidy; keep the full
  // text in the tooltip for the complete diagnostic.
  QString oneLine = error;
  oneLine.replace('\n', "  ").replace('\r', "");
  QFontMetrics fm(m_stageLabel->font());
  m_stageLabel->setText(fm.elidedText(oneLine, Qt::ElideRight, 260));
  Theme::setRole(m_stageLabel, "error");
  setToolTip(error);
  m_stageLabel->setToolTip(error);
}

void JobListItem::setOutput(const QString &path) {
  m_outputPath = path;
  m_showBtn->setEnabled(!path.isEmpty());
  m_showBtn->setToolTip(path);
}

int JobListItem::jobId() const { return m_jobId; }

JobListWidget::JobListWidget(QWidget *parent) : QWidget(parent) {
  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(0, 0, 0, 0);
  vl->setSpacing(0);

  m_list = new QListWidget(this);
  m_list->setObjectName("jobList");
  m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_list->setSelectionMode(QAbstractItemView::NoSelection);
  m_list->setFocusPolicy(Qt::NoFocus);
  vl->addWidget(m_list);
}

void JobListWidget::addJob(const Job &job) {
  if (m_widgetMap.contains(job.id))
    return;
  auto *item = new QListWidgetItem(m_list);
  auto *widget = new JobListItem(job, m_list);
  item->setSizeHint(QSize(0, widget->height()));
  m_list->addItem(item);
  m_list->setItemWidget(item, widget);
  m_itemMap[job.id] = item;
  m_widgetMap[job.id] = widget;
}

bool JobListWidget::hasJob(int jobId) const {
  return m_widgetMap.contains(jobId);
}

void JobListWidget::updateJobProgress(int jobId, int percent,
                                      const QString &stage) {
  if (auto *w = m_widgetMap.value(jobId))
    w->updateProgress(percent, stage);
}

void JobListWidget::updateJobStatus(int jobId, JobStatus status) {
  if (auto *w = m_widgetMap.value(jobId)) {
    w->setStatus(status);
    if (status != JobStatus::Queued && status != JobStatus::Completed &&
        status != JobStatus::Failed && status != JobStatus::Cancelled)
      m_list->scrollToItem(m_itemMap.value(jobId));
  }
}

void JobListWidget::setJobError(int jobId, const QString &error) {
  if (auto *w = m_widgetMap.value(jobId))
    w->setError(error);
}

void JobListWidget::setJobOutput(int jobId, const QString &path) {
  if (auto *w = m_widgetMap.value(jobId))
    w->setOutput(path);
}

void JobListWidget::cancelPending() {
  for (JobListItem *w : std::as_const(m_widgetMap))
    if (w->status() == JobStatus::Queued)
      w->setStatus(JobStatus::Cancelled);
}

void JobListWidget::clearAll() {
  m_list->clear();
  m_itemMap.clear();
  m_widgetMap.clear();
}

int JobListWidget::jobCount() const { return m_list->count(); }
