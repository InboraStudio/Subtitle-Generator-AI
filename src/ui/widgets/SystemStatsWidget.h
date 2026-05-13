#pragma once
#include "../../monitor/SystemMonitor.h"
#include "CircularProgressWidget.h"
#include <QLabel>
#include <QWidget>


class StatGauge : public QWidget {
  Q_OBJECT
public:
  explicit StatGauge(const QString &label, QWidget *parent = nullptr);
  void setValue(int percent, const QString &detail = QString());

private:
  CircularProgressWidget *m_circle;
  QLabel *m_label;
  QLabel *m_detail;
};

class SystemStatsWidget : public QWidget {
  Q_OBJECT
public:
  explicit SystemStatsWidget(QWidget *parent = nullptr);

public slots:
  void updateStats(const SystemStats &stats);

private:
  StatGauge *m_cpu;
  StatGauge *m_ram;
  StatGauge *m_gpu;
};
