#pragma once
#include "../../pipeline/Job.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
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

signals:
  void filesChanged(int count);
  void startRequested();

private slots:
  void addFile();
  void addFolder();
  void removeSelected();
  void clearAll();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QListWidget *m_fileList;
  QLabel *m_countLabel;
  QPushButton *m_addFileBtn;
  QPushButton *m_addFolderBtn;
  QPushButton *m_removeBtn;
  QPushButton *m_clearBtn;
  QPushButton *m_startBtn;

  QStringList m_filePaths;
  void addFilePaths(const QStringList &paths);
  void refreshList();
};
