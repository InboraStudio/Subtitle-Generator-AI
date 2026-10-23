#pragma once
#include "../../monitor/SystemMonitor.h"
#include <QFrame>
#include <QLabel>
#include <QProgressBar>
#include <QWidget>

// One "CPU [=====    ] 42%" meter in the stats strip.
class StatMeter : public QWidget {
  Q_OBJECT
public:
  explicit StatMeter(const QString &label, QWidget *parent = nullptr);
  void setValue(int percent, const QString &value, const QString &detail = {});
  void setUnavailable(const QString &reason);

private:
  QLabel *m_value;
  QLabel *m_detail;
  QProgressBar *m_bar;
};

// Compact live resource strip shown under every page.
class SystemStatsWidget : public QFrame {
  Q_OBJECT
public:
  explicit SystemStatsWidget(QWidget *parent = nullptr);

public slots:
  void updateStats(const SystemStats &stats);

private:
  StatMeter *m_cpu;
  StatMeter *m_ram;
  StatMeter *m_gpu;
};
