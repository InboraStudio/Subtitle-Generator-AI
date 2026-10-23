#include "SystemStatsWidget.h"
#include "../Theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

StatMeter::StatMeter(const QString &label, QWidget *parent) : QWidget(parent) {
  auto *hl = new QHBoxLayout(this);
  hl->setContentsMargins(0, 0, 0, 0);
  hl->setSpacing(10);

  auto *name = new QLabel(label, this);
  name->setObjectName("statName");
  name->setFixedWidth(30);
  hl->addWidget(name);

  m_bar = new QProgressBar(this);
  m_bar->setRange(0, 100);
  m_bar->setTextVisible(false);
  m_bar->setProperty("thin", true);
  m_bar->setMinimumWidth(60);
  hl->addWidget(m_bar, 1);

  m_value = new QLabel("--", this);
  m_value->setObjectName("statValue");
  m_value->setFixedWidth(44);
  m_value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  hl->addWidget(m_value);

  m_detail = new QLabel(this);
  m_detail->setObjectName("statDetail");
  m_detail->setMinimumWidth(90);
  hl->addWidget(m_detail);
}

void StatMeter::setValue(int percent, const QString &value,
                         const QString &detail) {
  m_bar->setEnabled(true);
  m_bar->setValue(percent);
  // Warn visually when a resource is close to saturation.
  const QString state = percent >= 90 ? "error" : percent >= 75 ? "warn" : "";
  if (m_bar->property("state").toString() != state) {
    m_bar->setProperty("state", state);
    Theme::repolish(m_bar);
  }
  m_value->setText(value);
  m_detail->setText(detail);
}

void StatMeter::setUnavailable(const QString &reason) {
  m_bar->setValue(0);
  m_bar->setEnabled(false);
  m_value->setText("N/A");
  m_detail->setText(reason);
}

SystemStatsWidget::SystemStatsWidget(QWidget *parent) : QFrame(parent) {
  setObjectName("statsStrip");
  setFixedHeight(40);

  auto *hl = new QHBoxLayout(this);
  hl->setContentsMargins(20, 0, 20, 0);
  hl->setSpacing(20);

  m_cpu = new StatMeter("CPU", this);
  m_ram = new StatMeter("RAM", this);
  m_gpu = new StatMeter("GPU", this);

  for (StatMeter *m : {m_cpu, m_ram, m_gpu}) {
    if (m != m_cpu) {
      auto *div = new QFrame(this);
      div->setObjectName("vDivider");
      div->setFixedHeight(18);
      hl->addWidget(div);
    }
    hl->addWidget(m, 1);
  }
}

void SystemStatsWidget::updateStats(const SystemStats &stats) {
  m_cpu->setValue(static_cast<int>(stats.cpuPercent),
                  QString("%1%").arg(stats.cpuPercent, 0, 'f', 0),
                  "processor");
  m_ram->setValue(static_cast<int>(stats.ramPercent),
                  QString("%1%").arg(stats.ramPercent, 0, 'f', 0),
                  QString("%1 / %2 GB")
                      .arg(stats.ramUsedMB / 1024.0, 0, 'f', 1)
                      .arg(stats.ramTotalMB / 1024.0, 0, 'f', 1));
  if (stats.gpuAvailable)
    m_gpu->setValue(static_cast<int>(stats.gpuPercent),
                    QString("%1%").arg(stats.gpuPercent, 0, 'f', 0),
                    stats.gpuName.isEmpty() ? QString("graphics")
                                            : stats.gpuName.left(22));
  else
    m_gpu->setUnavailable("not detected");
}
