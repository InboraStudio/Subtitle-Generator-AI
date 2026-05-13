#include "SystemStatsWidget.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>


StatGauge::StatGauge(const QString &label, QWidget *parent) : QWidget(parent) {
  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(8, 8, 8, 8);
  vl->setSpacing(4);
  vl->setAlignment(Qt::AlignHCenter);

  m_circle = new CircularProgressWidget(this);
  m_circle->setLabel(label);
  vl->addWidget(m_circle, 0, Qt::AlignHCenter);

  m_detail = new QLabel("--", this);
  m_detail->setObjectName("statusText");
  m_detail->setAlignment(Qt::AlignCenter);
  m_detail->setStyleSheet("color:#505050; font-size:7.5pt;");
  vl->addWidget(m_detail);
}

void StatGauge::setValue(int percent, const QString &detail) {
  m_circle->setValue(percent);
  m_detail->setText(detail);
}

SystemStatsWidget::SystemStatsWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("statsWidget");

  auto *hl = new QHBoxLayout(this);
  hl->setContentsMargins(12, 8, 12, 8);
  hl->setSpacing(0);

  m_cpu = new StatGauge("CPU", this);
  m_ram = new StatGauge("RAM", this);
  m_gpu = new StatGauge("GPU", this);

  for (auto *g : {m_cpu, m_ram, m_gpu}) {
    hl->addWidget(g, 1);
    if (g != m_gpu) {
      auto *div = new QFrame(this);
      div->setFrameShape(QFrame::VLine);
      div->setStyleSheet("background:#222222; max-width:1px;");
      hl->addWidget(div);
    }
  }
}

void SystemStatsWidget::updateStats(const SystemStats &stats) {
  m_cpu->setValue(static_cast<int>(stats.cpuPercent),
                  QString("%1%").arg(stats.cpuPercent, 0, 'f', 1));
  m_ram->setValue(
      static_cast<int>(stats.ramPercent),
      QString("%1/%2 MB").arg(stats.ramUsedMB).arg(stats.ramTotalMB));
  if (stats.gpuAvailable)
    m_gpu->setValue(static_cast<int>(stats.gpuPercent),
                    QString("%1%").arg(stats.gpuPercent, 0, 'f', 1));
  else
    m_gpu->setValue(0, "N/A");
}
