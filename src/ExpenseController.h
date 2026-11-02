#ifndef EXPENSE_CONTROLLER_H
#define EXPENSE_CONTROLLER_H

#include <optional>
#include <vector>

#include <QDate>
#include <QString>

#include "ActionResult.h"
#include "Budget.h"
#include "Expense.h"

class BudgetRepository;

// Feature: Recording money in and out (UR-4 to UR-8). Turns what the user typed
// on the Expenses page into changes on the Budget and saves each one. If saving
// fails, the Budget is put back so the screen and the database agree.
class ExpenseController {
public:
    // The view limits the text box to this, so the user hits the limit while typing.
    static constexpr int MAX_DESCRIPTION_LENGTH = Budget::MAX_DESCRIPTION_LENGTH;
    static constexpr const char* NO_TERM_MESSAGE = "Set up your term first on the Term setup tab.";

    ExpenseController(Budget& budget, BudgetRepository& repository);

    bool hasTerm() const;
    ActionResult addExpense(const QString& amountText, const QDate& date, const QString& description);
    ActionResult updateExpense(int expenseId, const QString& amountText, const QDate& date,
                               const QString& description);
    ActionResult removeExpense(int expenseId);
    ActionResult undoLastChange();
    bool canUndo() const;
    QString undoLabel() const;

    std::vector<Expense> expensesNewestFirst() const;
    std::optional<Expense> findExpense(int expenseId) const;
    // The date the form should show: the given day, moved inside the term if needed.
    QDate suggestedDate(const QDate& day) const;

private:
    Budget& budget;
    BudgetRepository& repository;

    QString describe(const Expense& expense) const;
    QString totalsSentence() const;
};

#endif
