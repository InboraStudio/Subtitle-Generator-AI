#pragma once
#include "../logging/LogModel.h"
#include "../monitor/MonitorWorker.h"
#include "../pipeline/BatchProcessor.h"
#include "../pipeline/JobQueue.h"
#include "SidebarWidget.h"
#include "panels/AboutPanel.h"
#include "panels/InputPanel.h"
#include "panels/ProgressPanel.h"
#include "panels/SettingsPanel.h"
#include "widgets/LogPanel.h"
#include "widgets/SystemStatsWidget.h"
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QWidget>

class MainWindow : public QWidget {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  // Queues videos/folders passed on the command line ("Open with", or files
  // dropped onto the executable).
  void openPaths(const QStringList &paths);

protected:
  bool eventFilter(QObject *obj, QEvent *e) override;
  void changeEvent(QEvent *e) override;
  void closeEvent(QCloseEvent *e) override;
  void resizeEvent(QResizeEvent *e) override;

private slots:
  void onTabSelected(int index);
  void onStartProcessing();
  void onPauseProcessing(bool paused);
  void onCancelProcessing();
  void onJobStarted(int id, const QString &path);
  void onJobFinished(int id, const QString &out);
  void onJobFailed(int id, const QString &err);
  void onAllComplete();
  void toggleLogPanel();

private:
  void setupLayout();
  void setupConnections();
  void setupShortcuts();
  void setBusy(bool busy);
  void setStatus(const QString &text, const QString &role = QString());
  void showToast(const QString &text, const QString &role = QString());
  void updateBadges();
  void finishIfIdle();
  bool confirmCanStart();
  void offerFirstModel();

  SidebarWidget *m_sidebar;
  QStackedWidget *m_stack;
  InputPanel *m_inputPanel;
  SettingsPanel *m_settingsPanel;
  ProgressPanel *m_progressPanel;
  AboutPanel *m_aboutPanel;
  SystemStatsWidget *m_statsWidget;
  LogPanel *m_logPanel;

  JobQueue *m_queue;
  BatchProcessor *m_processor;
  MonitorWorker *m_monitorWorker;
  LogModel *m_logModel;

  QFrame *m_titleBar;
  QLabel *m_pageLabel;
  QPushButton *m_consoleBtn;
  QPushButton *m_maxBtn;
  QLabel *m_statusLabel;
  QLabel *m_summaryLabel;
  QLabel *m_toast;
  QTimer m_toastTimer;

  bool m_busy = false;
  bool m_cancelling = false;
  int m_batchErrors = 0;
};
