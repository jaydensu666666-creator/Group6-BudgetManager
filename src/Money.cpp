#include "Money.h"

#include <cmath>

#include <QLocale>
#include <QRegularExpression>

namespace {

const QLocale US_ENGLISH(QLocale::English, QLocale::UnitedStates);

}  // namespace

namespace Money {

double roundToCents(double amount) {
    return std::round(amount * 100.0) / 100.0;
}

QString format(double amount) {
    const double rounded = roundToCents(amount);
    // A tiny negative like -0.001 rounds to zero and should not print as -$0.00.
    const bool negative = rounded < 0.0;
    const QString digits = US_ENGLISH.toString(std::fabs(rounded), 'f', 2);
    return (negative ? "-$" : "$") + digits;
}

bool parse(const QString& text, double& amount) {
    // Digits, commas and a point, with an optional minus and dollar sign in front.
    // This rules out things like "1e5" (a slip for 135) or "1$2".
    static const QRegularExpression PLAIN_AMOUNT("^-?\\$?[0-9.,]+$");
    QString cleaned = text.trimmed();
    if (!PLAIN_AMOUNT.match(cleaned).hasMatch()) {
        return false;
    }
    cleaned.remove('$');
    // The US locale accepts "1,200.50" but not "12,50", which a user in a
    // decimal-comma country might type and expect to mean twelve and a half.
    bool ok = false;
    const double value = US_ENGLISH.toDouble(cleaned, &ok);
    if (!ok || !std::isfinite(value)) {
        return false;
    }
    const double rounded = roundToCents(value);
    if (!std::isfinite(rounded)) {  // huge values like 1e307 overflow when rounded
        return false;
    }
    amount = rounded;
    return true;
}

}  // namespace Money
