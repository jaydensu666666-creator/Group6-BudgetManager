#ifndef EXPENSE_CONTROLLER_H
#define EXPENSE_CONTROLLER_H

#include <optional>
#include <vector>

#include <QDate>
#include <QString>

#include "Budget.h"
#include "Expense.h"

class BudgetRepository;

// What the user just did on the Expenses page.
enum class ExpenseAction { None, Added, Updated, Removed, Undone };

// Why a change on the Expenses page did not happen. The view turns each one into
// the sentence the user reads; the controller never builds text for the user
// (M2 feedback).
enum class ExpenseActionError {
    None,
    NoTerm,
    BadAmountText,  // the number the user typed could not be read as money
    AmountNotPositive,
    AmountTooLarge,
    InvalidDate,
    BeforeTermStart,
    OnOrAfterNextTransfer,
    DescriptionTooLong,
    DuplicateId,
    NotFound,
    NothingToUndo,
    SaveFailed,  // the database refused the change
};

// What happened, with the purchase and the numbers the view needs to write its
// own message.
struct ExpenseActionResult {
    bool ok = false;
    ExpenseAction action = ExpenseAction::None;
    ExpenseActionError error = ExpenseActionError::None;
    Expense expense;      // the purchase that was added, changed, removed or put back
    // When action is Undone, which kind of change was reversed, so the view can
    // say what came back.
    ExpenseChange::Type undoneChange = ExpenseChange::Type::Added;
    QString saveError;    // the database's reason, shown after "Couldn't save"
    QDate termStartDate;  // for the message about a date before the term
    QDate nextTransferDate;
    double totalSpent = 0.0;
    double remainingAmount = 0.0;
};

// Feature: Recording money in and out (UR-4 to UR-8). Turns what the user typed
// on the Expenses page into changes on the Budget and saves each one. If saving
// fails, the Budget is put back so the screen and the database agree.
class ExpenseController {
public:
    // The view limits the text box to this, so the user hits the limit while typing.
    static constexpr int MAX_DESCRIPTION_LENGTH = Budget::MAX_DESCRIPTION_LENGTH;

    ExpenseController(Budget& budget, BudgetRepository& repository);

    bool hasTerm() const;
    ExpenseActionResult addExpense(const QString& amountText, const QDate& date, const QString& description);
    ExpenseActionResult updateExpense(int expenseId, const QString& amountText, const QDate& date,
                                      const QString& description);
    ExpenseActionResult removeExpense(int expenseId);
    ExpenseActionResult undoLastChange();
    bool canUndo() const;
    // Which change Undo would reverse, so the view can label its button.
    std::optional<ExpenseChange::Type> lastChangeType() const;

    std::vector<Expense> expensesNewestFirst() const;
    std::optional<Expense> findExpense(int expenseId) const;
    // The date the form should show: the given day, moved inside the term if needed.
    QDate suggestedDate(const QDate& day) const;

private:
    Budget& budget;
    BudgetRepository& repository;

    // Starts a result with the current term's dates and numbers, which every
    // message on this page uses.
    ExpenseActionResult newResult() const;
};

#endif
