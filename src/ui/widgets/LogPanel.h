#pragma once
#include "../../logging/LogModel.h"
#include <QCheckBox>
#include <QListView>
#include <QPushButton>
#include <QWidget>


class LogPanel : public QWidget {
  Q_OBJECT
public:
  explicit LogPanel(LogModel *model, QWidget *parent = nullptr);

  void setVisible(bool visible) override;

public slots:
  void scrollToBottom();
  void saveToFile();
  void clearLog();

signals:
  void visibilityChanged(bool visible);

private:
  QListView *m_view;
  LogModel *m_model;
  QPushButton *m_saveBtn;
  QPushButton *m_clearBtn;
  QCheckBox *m_autoScrollChk;
  bool m_autoScroll = true;
};
