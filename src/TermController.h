#ifndef TERM_CONTROLLER_H
#define TERM_CONTROLLER_H

#include <QDate>
#include <QString>

#include "ActionResult.h"

class Budget;
class BudgetRepository;

// What the term setup page shows.
struct TermInfo {
    bool hasTerm = false;
    bool termOver = false;  // the next transfer date has passed
    double receivedAmount = 0.0;
    double savingsAmount = 0.0;
    QDate startDate;
    QDate nextTransferDate;
};

// Feature: Planning the term (UR-1 to UR-3). Turns what the user typed on the
// setup page into a term on the Budget and saves it.
class TermController {
public:
    TermController(Budget& budget, BudgetRepository& repository);

    ActionResult saveTerm(const QString& receivedText, const QString& savingsText,
                          const QDate& startDate, const QDate& nextTransferDate);
    TermInfo currentTerm(const QDate& today) const;

private:
    Budget& budget;
    BudgetRepository& repository;
};

#endif
