#include "LogPanel.h"
#include <QCheckBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextStream>
#include <QVBoxLayout>

class LogDelegate : public QStyledItemDelegate {
public:
  explicit LogDelegate(QObject *parent = nullptr)
      : QStyledItemDelegate(parent) {}
  void paint(QPainter *p, const QStyleOptionViewItem &opt,
             const QModelIndex &idx) const override {
    p->save();
    QColor bg = (opt.state & QStyle::State_Selected) ? QColor("#1E1E1E")
                                                     : QColor("#101010");
    p->fillRect(opt.rect, bg);
    QColor textColor = idx.data(LogModel::ColorRole).value<QColor>();
    p->setPen(textColor);
    QFont f;
    f.setFamily("Consolas");
    f.setPointSize(8);
    p->setFont(f);
    p->drawText(opt.rect.adjusted(8, 1, -8, -1),
                Qt::AlignVCenter | Qt::AlignLeft,
                idx.data(Qt::DisplayRole).toString());
    p->restore();
  }
  QSize sizeHint(const QStyleOptionViewItem &,
                 const QModelIndex &) const override {
    return QSize(100, 17);
  }
};

LogPanel::LogPanel(LogModel *model, QWidget *parent)
    : QWidget(parent), m_model(model) {
  setObjectName("logPanel");
  setMinimumHeight(140);
  setMaximumHeight(300);

  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(0, 0, 0, 0);
  vl->setSpacing(0);

  auto *toolbar = new QWidget(this);
  toolbar->setStyleSheet("background:#131313; border-bottom:1px solid #222;");
  toolbar->setFixedHeight(28);
  auto *tl = new QHBoxLayout(toolbar);
  tl->setContentsMargins(8, 0, 8, 0);
  tl->setSpacing(6);

  auto *titleLbl = new QLabel("Console", toolbar);
  titleLbl->setStyleSheet(
      "color:#505050; font-size:8pt; font-weight:600; letter-spacing:0.5px;");
  tl->addWidget(titleLbl);
  tl->addStretch();

  m_autoScrollChk = new QCheckBox("Auto-scroll", toolbar);
  m_autoScrollChk->setChecked(true);
  m_autoScrollChk->setStyleSheet("color:#454545; font-size:7.5pt;");
  connect(m_autoScrollChk, &QCheckBox::toggled,
          [this](bool on) { m_autoScroll = on; });
  tl->addWidget(m_autoScrollChk);

  m_saveBtn = new QPushButton("Save", toolbar);
  m_saveBtn->setObjectName("iconBtn");
  m_saveBtn->setStyleSheet(
      "font-size:7.5pt; color:#505050; background:transparent; border:none; "
      "min-width:34px; max-width:34px; min-height:20px; max-height:20px;");
  connect(m_saveBtn, &QPushButton::clicked, this, &LogPanel::saveToFile);
  tl->addWidget(m_saveBtn);

  m_clearBtn = new QPushButton("Clear", toolbar);
  m_clearBtn->setObjectName("iconBtn");
  m_clearBtn->setStyleSheet(
      "font-size:7.5pt; color:#505050; background:transparent; border:none; "
      "min-width:34px; max-width:34px; min-height:20px; max-height:20px;");
  connect(m_clearBtn, &QPushButton::clicked, this, &LogPanel::clearLog);
  tl->addWidget(m_clearBtn);

  vl->addWidget(toolbar);

  m_view = new QListView(this);
  m_view->setObjectName("logView");
  m_view->setModel(m_model);
  m_view->setItemDelegate(new LogDelegate(m_view));
  m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  vl->addWidget(m_view);

  connect(m_model, &QAbstractListModel::rowsInserted, this, [this] {
    if (m_autoScroll)
      scrollToBottom();
  });
}

void LogPanel::scrollToBottom() { m_view->scrollToBottom(); }

void LogPanel::saveToFile() {
  QString path = QFileDialog::getSaveFileName(
      this, "Save Log", QString(), "Text Files (*.txt);;All Files (*)");
  if (path.isEmpty())
    return;

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return;
  QTextStream out(&file);
  for (const LogEntry &e : m_model->entries()) {
    out << e.timestamp.toString("yyyy-MM-dd HH:mm:ss") << " " << e.message
        << "\n";
  }
}

void LogPanel::clearLog() { m_model->clear(); }

void LogPanel::setVisible(bool visible) {
  QWidget::setVisible(visible);
  emit visibilityChanged(visible);
}
