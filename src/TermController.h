#ifndef TERM_CONTROLLER_H
#define TERM_CONTROLLER_H

#include <QDate>
#include <QString>

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

// Why the term was not saved. The view turns each one into the sentence the user
// reads; the controller never builds text for the user (M2 feedback).
enum class TermSaveError {
    None,
    BadReceivedText,  // the number the user typed could not be read as money
    BadSavingsText,
    TransferNotArrived,  // the next term may not start before the transfer arrives
    ReceivedNotPositive,
    AmountTooLarge,
    SavingsNegative,
    SavingsMoreThanReceived,
    MissingDate,
    TransferNotAfterStart,
    ExpensesOutsideTerm,
    SaveFailed,  // the database refused the change
};

// What happened when the user pressed Save term, with the numbers and dates the
// view needs to write its own message.
struct TermSaveResult {
    bool ok = false;
    bool startedNewTerm = false;
    TermSaveError error = TermSaveError::None;
    QDate currentTermEnd;    // last day of the term that is still running
    QDate nextTransferDate;  // when that term's transfer arrives
    double spendableAmount = 0.0;
    double remainingAmount = 0.0;
    QString saveError;  // the database's reason, shown after "Couldn't save"
};

// Feature: Planning the term (UR-1 to UR-3). Turns what the user typed on the
// setup page into a term on the Budget and saves it. A start date on or after
// the current next transfer date starts the next term, which is saved as its own
// budget row: the last term's purchases and undo step stay with the last term.
class TermController {
public:
    TermController(Budget& budget, BudgetRepository& repository);

    TermSaveResult saveTerm(const QString& receivedText, const QString& savingsText,
                            const QDate& startDate, const QDate& nextTransferDate);
    TermInfo currentTerm(const QDate& today) const;

private:
    Budget& budget;
    BudgetRepository& repository;
};

#endif
