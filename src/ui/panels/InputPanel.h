#pragma once
#include "../../pipeline/Job.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QWidget>

class InputPanel : public QWidget {
  Q_OBJECT
public:
  explicit InputPanel(QWidget *parent = nullptr);

  QList<Job> buildJobs(const QString &outputDir, const QString &modelPath,
                       const QString &srcLang, const QString &tgtLang,
                       bool translate, bool embedSubs, bool fastMode,
                       const QString &fontName, int fontSize,
                       const QString &fontColor, int outlineThickness,
                       int alignment) const;
  int fileCount() const;

  // Adds files and folders (scanned recursively), e.g. from the command line
  // or a drop.
  void addPaths(const QStringList &paths);

  // Locks the queue while a batch is running so it cannot change underneath.
  void setBusy(bool busy);

public slots:
  void addFile();
  void addFolder();

signals:
  void filesChanged(int count);
  void startRequested();

private slots:
  void removeSelected();
  void clearAll();
  void showContextMenu(const QPoint &pos);
  void updateActions();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QStackedWidget *m_stack;
  QFrame *m_dropZone;
  QTableWidget *m_table;
  QLabel *m_countLabel;
  QLabel *m_summaryLabel;
  QPushButton *m_addFileBtn;
  QPushButton *m_addFolderBtn;
  QPushButton *m_removeBtn;
  QPushButton *m_clearBtn;
  QPushButton *m_startBtn;
  bool m_busy = false;

  QStringList m_filePaths;
  void addFilePaths(const QStringList &paths);
  void refreshList();
  void setDragActive(bool active);
};
