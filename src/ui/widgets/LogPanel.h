#pragma once
#include "../../logging/LogModel.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSortFilterProxyModel>

// Filters console lines by minimum level and free text.
class LogFilterProxy : public QSortFilterProxyModel {
  Q_OBJECT
public:
  using QSortFilterProxyModel::QSortFilterProxyModel;
  void setMinLevel(int level);
  void setText(const QString &text);

protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
  int m_minLevel = 0;
  QString m_text;
};

class LogPanel : public QFrame {
  Q_OBJECT
public:
  explicit LogPanel(LogModel *model, QWidget *parent = nullptr);

  void setVisible(bool visible) override;

public slots:
  void scrollToBottom();
  void saveToFile();
  void clearLog();
  void copySelection();

signals:
  void visibilityChanged(bool visible);
  void closeRequested();

private:
  QListView *m_view;
  LogModel *m_model;
  LogFilterProxy *m_proxy;
  QComboBox *m_levelCombo;
  QLineEdit *m_filterEdit;
  QCheckBox *m_autoScrollChk;
  bool m_autoScroll = true;
};
