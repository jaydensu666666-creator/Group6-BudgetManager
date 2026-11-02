#ifndef MONEY_H
#define MONEY_H

#include <QString>

// Helpers for reading and showing dollar amounts. Every amount in the app goes
// through these, so the rules for cents and minus signs live in one place.
// Money holds no data; views use it only to format amounts for display.
namespace Money {

// Largest amount the app accepts for one expense or one term, so a typo with
// extra zeros gets a message instead of being saved.
constexpr double MAX_AMOUNT = 1000000.0;

// Rounds to whole cents so sums like 0.1 + 0.2 do not show stray digits.
double roundToCents(double amount);

// Formats as US dollars with the minus sign before the dollar sign,
// for example -$2,500.00 (M1 feedback: we used to show $-2500).
QString format(double amount);

// Reads text like "12", "12.5", "-3", "$1,200.00" and rounds it to cents.
// Commas are only allowed as thousands separators, so "12,50" is rejected
// instead of being read as 1250. Returns false for anything that is not a
// plain number, including NaN and infinity. Negative numbers are read so the
// model can tell the user why they are not allowed.
bool parse(const QString& text, double& amount);

}  // namespace Money

#endif
