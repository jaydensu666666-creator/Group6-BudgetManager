#ifndef STATUS_CONTROLLER_H
#define STATUS_CONTROLLER_H

#include <QDate>

class Budget;

// Everything the home page shows, worked out for one day.
struct SpendingStatus {
    bool hasTerm = false;
    bool termOver = false;
    double receivedAmount = 0.0;
    double savingsAmount = 0.0;
    double spendableAmount = 0.0;
    double totalSpent = 0.0;
    double moneyLeft = 0.0;
    QDate nextTransferDate;
    int daysUntilNextTransfer = 0;
    double spendingPerDay = 0.0;
    bool willLast = true;
    QDate runOutDate;
    double weeklyAllowance = 0.0;
    double spentThisWeek = 0.0;
    double leftThisWeek = 0.0;
};

// Feature: Checking where they stand (UR-9 to UR-11). Reads the numbers
// from the Budget for the home page. It never changes anything.
class StatusController {
public:
    explicit StatusController(const Budget& budget);

    SpendingStatus status(const QDate& today) const;

private:
    const Budget& budget;
};

#endif
