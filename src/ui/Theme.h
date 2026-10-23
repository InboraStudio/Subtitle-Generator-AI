#pragma once
#include <QColor>
#include <QIcon>
#include <QString>

class QApplication;
class QFrame;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;
class QWidget;

// Single source of truth for the app's look. The palette mirrors the Model
// Manager dialog: near-black surfaces, a #1C1C1C header strip, muted grey
// secondary text and one orange accent. Static styling lives in dark.qss and
// is selected through object names and the "role" property set here, so no
// widget needs an inline stylesheet.
namespace Theme {

namespace Color {
inline const QColor Window("#141414");
inline const QColor Surface("#161616");
inline const QColor SurfaceRaised("#1C1C1C");
inline const QColor Border("#262626");
inline const QColor Track("#252525");
inline const QColor Text("#D0D0D0");
inline const QColor TextStrong("#E0E0E0");
inline const QColor TextMuted("#808080");
inline const QColor TextFaint("#505050");
inline const QColor Accent("#E8872A");
inline const QColor Success("#4CAF50");
inline const QColor Warning("#E0A030");
inline const QColor Error("#E05050");
} // namespace Color

// Loads dark.qss and a matching dark palette (for dialogs Qt draws itself).
void apply(QApplication &app);

// Sets the QSS "role" property on a label/widget and re-polishes it so the
// change shows immediately (role: pageTitle, subtitle, muted, faint, accent,
// success, warning, error, fieldLabel, hint, badge, statValue, statLabel).
void setRole(QWidget *w, const QString &role);

// Re-applies the stylesheet after a dynamic property changed.
void repolish(QWidget *w);

// The white line icons in resources, recoloured for the dark UI.
QIcon icon(const QString &name, const QColor &color = Color::TextMuted);

// Line glyphs drawn in code: window controls for the custom title bar
// ("min", "max", "restore", "close") and "info".
QIcon glyph(const QString &kind, const QColor &color);

// Standard page header: big title on the left, muted detail on the right.
// Returns the layout so callers can append actions; *detail receives the
// right-hand label when requested.
QHBoxLayout *pageHeader(QWidget *parent, const QString &title,
                        QLabel **detail = nullptr);

// A rounded surface with a Model-Manager-style header strip. Returns the
// card; *body receives the padded content layout.
QFrame *card(QWidget *parent, const QString &title, QVBoxLayout **body,
             QHBoxLayout **headerActions = nullptr);

// "Label ........ control" row used by every settings form so the labels
// share one column width.
QHBoxLayout *formRow(QWidget *parent, const QString &label, QWidget *control,
                     bool expand, const QString &tooltip = QString());

QString formatBytes(qint64 bytes);
QString formatDuration(int seconds);

} // namespace Theme
