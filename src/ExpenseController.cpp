#include "ExpenseController.h"

#include <QLocale>

#include "BudgetRepository.h"
#include "Money.h"

namespace {

const char* const BAD_AMOUNT_MESSAGE = "Enter the amount as a number, like 12.50.";

QString shortDate(const QDate& date) {
    return QLocale(QLocale::English).toString(date, "MMM d, yyyy");
}

QString expenseErrorMessage(ExpenseError error, const Budget& budget) {
    switch (error) {
    case ExpenseError::AmountNotPositive:
        return "Enter an amount greater than $0, like 12.50.";
    case ExpenseError::AmountTooLarge:
        return "That amount is too large. Enter up to " + Money::format(Money::MAX_AMOUNT) + ".";
    case ExpenseError::InvalidDate:
        return "Pick a date for this expense.";
    case ExpenseError::BeforeTermStart:
        return "This date is before your term started (" + shortDate(budget.getStartDate()) + ").";
    case ExpenseError::OnOrAfterNextTransfer:
        return "This date is on or after your next transfer (" + shortDate(budget.getNextTransferDate()) +
               "). Set up your new term first.";
    case ExpenseError::DescriptionTooLong:
        return QString("Keep the description to %1 characters or fewer.").arg(Budget::MAX_DESCRIPTION_LENGTH);
    case ExpenseError::DuplicateId:
    case ExpenseError::NotFound:
        return "That expense is no longer in your list.";
    case ExpenseError::NothingToUndo:
        return "There is nothing to undo.";
    case ExpenseError::None:
        break;
    }
    return QString();
}

}  // namespace

ExpenseController::ExpenseController(Budget& budget, BudgetRepository& repository)
    : budget(budget), repository(repository) {}

bool ExpenseController::hasTerm() const {
    return budget.hasTerm();
}

QString ExpenseController::describe(const Expense& expense) const {
    QString text = Money::format(expense.amount);
    if (!expense.description.isEmpty()) {
        text += " for " + expense.description;
    }
    return text + " on " + shortDate(expense.date);
}

QString ExpenseController::totalsSentence() const {
    // Shown right after every change so the user sees what they spent without
    // switching tabs (M1 feedback).
    return " Spent so far: " + Money::format(budget.getTotalExpenses()) +
           ". Left to spend: " + Money::format(budget.getRemainingBudget()) + ".";
}

ActionResult ExpenseController::addExpense(const QString& amountText, const QDate& date,
                                           const QString& description) {
    if (!budget.hasTerm()) {
        return ActionResult::failure(NO_TERM_MESSAGE);
    }
    Expense expense;
    if (!Money::parse(amountText, expense.amount)) {
        return ActionResult::failure(BAD_AMOUNT_MESSAGE);
    }
    expense.id = budget.nextExpenseId();
    expense.date = date;
    expense.description = description;

    const Budget before = budget;
    const ExpenseError error = budget.addExpense(expense);
    if (error != ExpenseError::None) {
        return ActionResult::failure(expenseErrorMessage(error, budget));
    }
    const Expense stored = *budget.findExpense(expense.id);
    if (!repository.insertExpense(stored)) {
        budget = before;  // keep the screen and the database telling the same story
        return ActionResult::failure("Couldn't save the expense: " + repository.lastError());
    }
    return ActionResult::success("Added " + describe(stored) + "." + totalsSentence());
}

ActionResult ExpenseController::updateExpense(int expenseId, const QString& amountText, const QDate& date,
                                              const QString& description) {
    std::optional<Expense> changed = budget.findExpense(expenseId);
    if (!changed) {
        return ActionResult::failure(expenseErrorMessage(ExpenseError::NotFound, budget));
    }
    if (!Money::parse(amountText, changed->amount)) {
        return ActionResult::failure(BAD_AMOUNT_MESSAGE);
    }
    changed->date = date;
    changed->description = description;

    const Budget before = budget;
    const ExpenseError error = budget.updateExpense(*changed);
    if (error != ExpenseError::None) {
        return ActionResult::failure(expenseErrorMessage(error, budget));
    }
    const Expense stored = *budget.findExpense(expenseId);
    if (!repository.updateExpense(stored)) {
        budget = before;
        return ActionResult::failure("Couldn't save the change: " + repository.lastError());
    }
    return ActionResult::success("Changed to " + describe(stored) + "." + totalsSentence());
}

ActionResult ExpenseController::removeExpense(int expenseId) {
    const std::optional<Expense> removed = budget.findExpense(expenseId);
    if (!removed) {
        return ActionResult::failure(expenseErrorMessage(ExpenseError::NotFound, budget));
    }
    const Budget before = budget;
    budget.removeExpense(expenseId);
    if (!repository.deleteExpense(expenseId)) {
        budget = before;
        return ActionResult::failure("Couldn't remove the expense: " + repository.lastError());
    }
    return ActionResult::success("Removed " + describe(*removed) + ". Press Undo to bring it back." +
                                 totalsSentence());
}

ActionResult ExpenseController::undoLastChange() {
    const std::optional<ExpenseChange> change = budget.getLastChange();
    if (!change) {
        return ActionResult::failure(expenseErrorMessage(ExpenseError::NothingToUndo, budget));
    }
    const Budget before = budget;
    const ExpenseError error = budget.undoLastChange();
    if (error != ExpenseError::None) {
        return ActionResult::failure("Can't undo: " + expenseErrorMessage(error, budget));
    }

    const Expense& expense = change->before;
    bool saved = false;
    QString message;
    switch (change->type) {
    case ExpenseChange::Type::Added:
        saved = repository.deleteExpense(expense.id);
        message = "Undone. " + describe(expense) + " was taken out again.";
        break;
    case ExpenseChange::Type::Edited:
        saved = repository.updateExpense(expense);
        message = "Undone. The expense is back to " + describe(expense) + ".";
        break;
    case ExpenseChange::Type::Removed:
        saved = repository.insertExpense(expense);
        message = "Undone. " + describe(expense) + " is back in your list.";
        break;
    }
    if (!saved) {
        budget = before;
        return ActionResult::failure("Couldn't undo: " + repository.lastError());
    }
    return ActionResult::success(message + totalsSentence());
}

bool ExpenseController::canUndo() const {
    return budget.getLastChange().has_value();
}

QString ExpenseController::undoLabel() const {
    const std::optional<ExpenseChange> change = budget.getLastChange();
    if (!change) {
        return "Undo";
    }
    switch (change->type) {
    case ExpenseChange::Type::Added:
        return "Undo add";
    case ExpenseChange::Type::Edited:
        return "Undo edit";
    case ExpenseChange::Type::Removed:
        return "Undo remove";
    }
    return "Undo";
}

std::vector<Expense> ExpenseController::expensesNewestFirst() const {
    return budget.termExpensesNewestFirst();
}

std::optional<Expense> ExpenseController::findExpense(int expenseId) const {
    return budget.findExpense(expenseId);
}

QDate ExpenseController::suggestedDate(const QDate& day) const {
    return budget.suggestedExpenseDate(day);
}
