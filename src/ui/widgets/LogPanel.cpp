#include "LogPanel.h"
#include "../Theme.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextStream>
#include <QVBoxLayout>
#include <algorithm>

class LogDelegate : public QStyledItemDelegate {
public:
  explicit LogDelegate(QObject *parent = nullptr)
      : QStyledItemDelegate(parent) {
    m_font.setFamilies({"Consolas", "Cascadia Mono", "Courier New"});
    m_font.setPointSizeF(8.5);
  }
  void paint(QPainter *p, const QStyleOptionViewItem &opt,
             const QModelIndex &idx) const override {
    p->save();
    QColor bg = (opt.state & QStyle::State_Selected) ? QColor("#1F1A12")
                : (opt.state & QStyle::State_MouseOver) ? QColor("#161616")
                                                         : QColor("#101010");
    p->fillRect(opt.rect, bg);
    p->setPen(idx.data(LogModel::ColorRole).value<QColor>());
    p->setFont(m_font);
    // Multi-line messages (FFmpeg output) get one row per line, each elided
    // to the view width, so nothing overlaps the next entry.
    const QFontMetrics fm(m_font);
    const QStringList lines = idx.data(Qt::DisplayRole).toString().split('\n');
    const int w = opt.rect.width() - 20;
    int y = opt.rect.top() + 2;
    for (const QString &line : lines) {
      p->drawText(QRect(opt.rect.left() + 12, y, w, fm.lineSpacing()),
                  Qt::AlignVCenter | Qt::AlignLeft,
                  fm.elidedText(line, Qt::ElideRight, w));
      y += fm.lineSpacing();
    }
    p->restore();
  }
  QSize sizeHint(const QStyleOptionViewItem &,
                 const QModelIndex &idx) const override {
    const int lines =
        idx.data(Qt::DisplayRole).toString().count('\n') + 1;
    return QSize(100, QFontMetrics(m_font).lineSpacing() * lines + 4);
  }

private:
  QFont m_font;
};

// invalidateFilter() is deprecated from Qt 6.10 in favour of the
// begin/endFilterChange() pair, which older Qt 6 releases lack.
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#define LOG_FILTER_CHANGE(stmt)                                                \
  beginFilterChange();                                                         \
  stmt;                                                                        \
  endFilterChange(QSortFilterProxyModel::Direction::Rows)
#else
#define LOG_FILTER_CHANGE(stmt)                                                \
  stmt;                                                                        \
  invalidateFilter()
#endif

void LogFilterProxy::setMinLevel(int level) {
  LOG_FILTER_CHANGE(m_minLevel = level);
}

void LogFilterProxy::setText(const QString &text) {
  LOG_FILTER_CHANGE(m_text = text);
}

bool LogFilterProxy::filterAcceptsRow(int row,
                                      const QModelIndex &parent) const {
  const QModelIndex idx = sourceModel()->index(row, 0, parent);
  if (idx.data(LogModel::LevelRole).toInt() < m_minLevel)
    return false;
  return m_text.isEmpty() ||
         idx.data(Qt::DisplayRole).toString().contains(m_text,
                                                       Qt::CaseInsensitive);
}

LogPanel::LogPanel(LogModel *model, QWidget *parent)
    : QFrame(parent), m_model(model) {
  setObjectName("logPanel");
  setMinimumHeight(160);
  setMaximumHeight(320);

  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(0, 0, 0, 0);
  vl->setSpacing(0);

  auto *toolbar = new QFrame(this);
  toolbar->setObjectName("logToolbar");
  toolbar->setFixedHeight(34);
  auto *tl = new QHBoxLayout(toolbar);
  tl->setContentsMargins(14, 0, 6, 0);
  tl->setSpacing(8);

  auto *titleLbl = new QLabel("CONSOLE", toolbar);
  titleLbl->setObjectName("cardTitle");
  tl->addWidget(titleLbl);
  tl->addSpacing(8);

  m_levelCombo = new QComboBox(toolbar);
  m_levelCombo->addItems({"All levels", "Info and above", "Warnings and errors",
                          "Errors only"});
  m_levelCombo->setCurrentIndex(1);
  m_levelCombo->setObjectName("logLevel");
  tl->addWidget(m_levelCombo);

  m_filterEdit = new QLineEdit(toolbar);
  m_filterEdit->setObjectName("logFilter");
  m_filterEdit->setPlaceholderText("Filter...");
  m_filterEdit->setClearButtonEnabled(true);
  m_filterEdit->setFixedWidth(180);
  tl->addWidget(m_filterEdit);
  tl->addStretch();

  m_autoScrollChk = new QCheckBox("Auto-scroll", toolbar);
  m_autoScrollChk->setChecked(true);
  connect(m_autoScrollChk, &QCheckBox::toggled, this, [this](bool on) {
    m_autoScroll = on;
    if (on)
      scrollToBottom();
  });
  tl->addWidget(m_autoScrollChk);

  auto addTool = [&](const QString &text, const QString &tip, auto slot) {
    auto *b = new QPushButton(text, toolbar);
    b->setObjectName("ghostBtn");
    b->setToolTip(tip);
    connect(b, &QPushButton::clicked, this, slot);
    tl->addWidget(b);
    return b;
  };
  addTool("Copy", "Copy selected lines (Ctrl+C)", &LogPanel::copySelection);
  addTool("Save", "Save the full log to a text file", &LogPanel::saveToFile);
  addTool("Clear", "Clear the console", &LogPanel::clearLog);
  auto *closeBtn = new QPushButton(toolbar);
  closeBtn->setObjectName("ghostBtn");
  closeBtn->setIcon(Theme::glyph("close", Theme::Color::TextMuted));
  closeBtn->setIconSize(QSize(10, 10));
  closeBtn->setToolTip("Hide console (Ctrl+`)");
  connect(closeBtn, &QPushButton::clicked, this, &LogPanel::closeRequested);
  tl->addWidget(closeBtn);

  vl->addWidget(toolbar);

  m_proxy = new LogFilterProxy(this);
  m_proxy->setSourceModel(m_model);
  m_proxy->setMinLevel(1);
  connect(m_levelCombo, &QComboBox::currentIndexChanged, m_proxy,
          &LogFilterProxy::setMinLevel);
  connect(m_filterEdit, &QLineEdit::textChanged, m_proxy,
          &LogFilterProxy::setText);

  m_view = new QListView(this);
  m_view->setObjectName("logView");
  m_view->setModel(m_proxy);
  m_view->setItemDelegate(new LogDelegate(m_view));
  m_view->setMouseTracking(true);
  m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  vl->addWidget(m_view);

  auto *copy = new QShortcut(QKeySequence::Copy, m_view);
  copy->setContext(Qt::WidgetShortcut);
  connect(copy, &QShortcut::activated, this, &LogPanel::copySelection);

  connect(m_proxy, &QAbstractItemModel::rowsInserted, this, [this] {
    if (m_autoScroll && isVisible())
      scrollToBottom();
  });
}

void LogPanel::scrollToBottom() { m_view->scrollToBottom(); }

void LogPanel::copySelection() {
  QModelIndexList rows = m_view->selectionModel()->selectedRows();
  if (rows.isEmpty())
    return;
  std::sort(rows.begin(), rows.end());
  QStringList lines;
  for (const QModelIndex &i : rows)
    lines << i.data(Qt::DisplayRole).toString();
  QApplication::clipboard()->setText(lines.join('\n'));
}

void LogPanel::saveToFile() {
  QString path = QFileDialog::getSaveFileName(
      this, "Save Log",
      QString("subtitle-generator-%1.log")
          .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")),
      "Log Files (*.log *.txt);;All Files (*)");
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
  QFrame::setVisible(visible);
  if (visible && m_autoScroll)
    scrollToBottom();
  emit visibilityChanged(visible);
}
