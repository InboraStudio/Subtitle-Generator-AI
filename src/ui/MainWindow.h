#pragma once
#include "../logging/LogModel.h"
#include "../monitor/MonitorWorker.h"
#include "../pipeline/BatchProcessor.h"
#include "../pipeline/JobQueue.h"
#include "SidebarWidget.h"
#include "panels/InputPanel.h"
#include "panels/ProgressPanel.h"
#include "panels/SettingsPanel.h"
#include "widgets/LogPanel.h"
#include "widgets/SystemStatsWidget.h"
#include <QLabel>
#include <QStackedWidget>
#include <QWidget>


class MainWindow : public QWidget {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void closeEvent(QCloseEvent *e) override;

private slots:
  void onTabSelected(int index);
  void onStartProcessing();
  void onPauseProcessing();
  void onCancelProcessing();
  void onJobStarted(int id, const QString &path);
  void onJobProgress(int id, int percent, const QString &stage);
  void onJobFinished(int id, const QString &out);
  void onJobFailed(int id, const QString &err);
  void onAllComplete();
  void toggleLogPanel();
  void updateStatusBar(const SystemStats &stats);

private:
  void setupLayout();
  void setupConnections();
  void loadStylesheet();
  void applyWindowFlags();
  QString resolveAppPath(const QString &relative) const;

  SidebarWidget *m_sidebar;
  QStackedWidget *m_stack;
  InputPanel *m_inputPanel;
  SettingsPanel *m_settingsPanel;
  ProgressPanel *m_progressPanel;
  SystemStatsWidget *m_statsWidget;
  LogPanel *m_logPanel;

  JobQueue *m_queue;
  BatchProcessor *m_processor;
  MonitorWorker *m_monitorWorker;
  LogModel *m_logModel;

  QLabel *m_statusLabel;
  QWidget *m_titleBar;

  bool m_dragging = false;
  QPoint m_dragStart;
  bool m_logVisible = false;
};
