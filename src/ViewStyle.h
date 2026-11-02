#ifndef VIEW_STYLE_H
#define VIEW_STYLE_H

#include <QString>

// Colors and formats shared by the views, so every page looks the same.
namespace ViewStyle {

inline const QString OK_COLOR = "#1e7d32";      // green: saved, money lasts
inline const QString ERROR_COLOR = "#c0392b";   // red: rejected, overspent
inline const QString MUTED_COLOR = "#555555";   // gray: captions and details
inline const QString DATE_FORMAT = "MMM d, yyyy";

// Long enough for "$1,000,000.00" with spaces around it; anything longer is a typo.
constexpr int MONEY_TEXT_MAX_LENGTH = 20;

inline QString messageStyle(bool ok) {
    return "color: " + (ok ? OK_COLOR : ERROR_COLOR) + ";";
}

}  // namespace ViewStyle

#endif
