#include "JobListWidget.h"
#include <QColor>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
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
    return "Completed";
  case JobStatus::Failed:
    return "Failed";
  case JobStatus::Cancelled:
    return "Cancelled";
  }
  return "Unknown";
}

static QColor statusColor(JobStatus s) {
  switch (s) {
  case JobStatus::Completed:
    return QColor("#4CAF50");
  case JobStatus::Failed:
    return QColor("#E05050");
  case JobStatus::Cancelled:
    return QColor("#808080");
  default:
    return QColor("#E8872A");
  }
}

JobListItem::JobListItem(const Job &job, QWidget *parent)
    : QWidget(parent), m_jobId(job.id) {
  setFixedHeight(52);
  setStyleSheet("background:#1C1C1C; border-bottom:1px solid #222222;");

  auto *hl = new QHBoxLayout(this);
  hl->setContentsMargins(10, 6, 10, 6);
  hl->setSpacing(10);

  auto *left = new QWidget(this);
  auto *vl = new QVBoxLayout(left);
  vl->setContentsMargins(0, 0, 0, 0);
  vl->setSpacing(2);

  m_nameLabel = new QLabel(QFileInfo(job.inputPath).fileName(), left);
  m_nameLabel->setStyleSheet(
      "color:#C8C8C8; font-size:8.5pt; font-weight:600;");
  m_nameLabel->setMaximumWidth(350);
  m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  QFontMetrics fm(m_nameLabel->font());
  m_nameLabel->setText(
      fm.elidedText(m_nameLabel->text(), Qt::ElideMiddle, 350));
  vl->addWidget(m_nameLabel);

  m_bar = new QProgressBar(left);
  m_bar->setRange(0, 100);
  m_bar->setValue(0);
  m_bar->setTextVisible(false);
  m_bar->setFixedHeight(4);
  vl->addWidget(m_bar);

  hl->addWidget(left, 1);

  auto *right = new QWidget(this);
  auto *vr = new QVBoxLayout(right);
  vr->setContentsMargins(0, 0, 0, 0);
  vr->setSpacing(2);
  vr->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  right->setFixedWidth(90);

  m_statusLabel = new QLabel("Queued", right);
  m_statusLabel->setStyleSheet(
      "color:#E8872A; font-size:7.5pt; font-weight:600;");
  m_statusLabel->setAlignment(Qt::AlignRight);
  vr->addWidget(m_statusLabel);

  m_stageLabel = new QLabel("", right);
  m_stageLabel->setStyleSheet("color:#454545; font-size:7pt;");
  m_stageLabel->setAlignment(Qt::AlignRight);
  vr->addWidget(m_stageLabel);

  hl->addWidget(right);
}

void JobListItem::updateProgress(int percent, const QString &stage) {
  m_bar->setValue(percent);
  m_stageLabel->setText(stage);
}

void JobListItem::setStatus(JobStatus status) {
  m_statusLabel->setText(statusToString(status));
  m_statusLabel->setStyleSheet(
      QString("color:%1; font-size:7.5pt; font-weight:600;")
          .arg(statusColor(status).name()));
  if (status == JobStatus::Completed)
    m_bar->setValue(100);
  if (status == JobStatus::Failed)
    m_bar->setValue(0);
}

void JobListItem::setError(const QString &error) {
  // Collapse newlines so the one-line stage label stays tidy; keep the full
  // text in the tooltip for the complete diagnostic.
  QString oneLine = error;
  oneLine.replace('\n', "  ").replace('\r', "");
  QFontMetrics fm(m_stageLabel->font());
  m_stageLabel->setText(fm.elidedText(oneLine, Qt::ElideRight, 200));
  m_stageLabel->setStyleSheet("color:#E05050; font-size:7pt;");
  setToolTip(error);
  m_stageLabel->setToolTip(error);
}

int JobListItem::jobId() const { return m_jobId; }

JobListWidget::JobListWidget(QWidget *parent) : QWidget(parent) {
  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(0, 0, 0, 0);
  vl->setSpacing(0);

  m_list = new QListWidget(this);
  m_list->setStyleSheet("QListWidget { background:#181818; border:none; }"
                        "QListWidget::item { padding:0; border:none; }");
  m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_list->setSelectionMode(QAbstractItemView::NoSelection);
  vl->addWidget(m_list);
}

void JobListWidget::addJob(const Job &job) {
  auto *item = new QListWidgetItem(m_list);
  auto *widget = new JobListItem(job, m_list);
  item->setSizeHint(widget->sizeHint());
  m_list->addItem(item);
  m_list->setItemWidget(item, widget);
  m_itemMap[job.id] = item;
  m_widgetMap[job.id] = widget;
}

void JobListWidget::updateJobProgress(int jobId, int percent,
                                      const QString &stage) {
  if (m_widgetMap.contains(jobId))
    m_widgetMap[jobId]->updateProgress(percent, stage);
}

void JobListWidget::updateJobStatus(int jobId, JobStatus status) {
  if (m_widgetMap.contains(jobId))
    m_widgetMap[jobId]->setStatus(status);
}

void JobListWidget::setJobError(int jobId, const QString &error) {
  if (m_widgetMap.contains(jobId))
    m_widgetMap[jobId]->setError(error);
}

void JobListWidget::clearAll() {
  m_list->clear();
  m_itemMap.clear();
  m_widgetMap.clear();
}

int JobListWidget::jobCount() const { return m_list->count(); }
  