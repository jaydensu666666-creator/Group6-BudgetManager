#include "ExpenseController.h"

#include "BudgetRepository.h"
#include "Money.h"

namespace {

// The model's reasons are part of what the view can show; this keeps the
// controller's result free of any sentence.
ExpenseActionError toActionError(ExpenseError error) {
    switch (error) {
    case ExpenseError::AmountNotPositive:
        return ExpenseActionError::AmountNotPositive;
    case ExpenseError::AmountTooLarge:
        return ExpenseActionError::AmountTooLarge;
    case ExpenseError::InvalidDate:
        return ExpenseActionError::InvalidDate;
    case ExpenseError::BeforeTermStart:
        return ExpenseActionError::BeforeTermStart;
    case ExpenseError::OnOrAfterNextTransfer:
        return ExpenseActionError::OnOrAfterNextTransfer;
    case ExpenseError::DescriptionTooLong:
        return ExpenseActionError::DescriptionTooLong;
    case ExpenseError::DuplicateId:
        return ExpenseActionError::DuplicateId;
    case ExpenseError::NotFound:
        return ExpenseActionError::NotFound;
    case ExpenseError::NothingToUndo:
        return ExpenseActionError::NothingToUndo;
    case ExpenseError::None:
        break;
    }
    return ExpenseActionError::None;
}

}  // namespace

ExpenseController::ExpenseController(Budget& budget, BudgetRepository& repository)
    : budget(budget), repository(repository) {}

bool ExpenseController::hasTerm() const {
    return budget.hasTerm();
}

ExpenseActionResult ExpenseController::newResult() const {
    ExpenseActionResult result;
    result.termStartDate = budget.getStartDate();
    result.nextTransferDate = budget.getNextTransferDate();
    result.totalSpent = budget.getTotalExpenses();
    result.remainingAmount = budget.getRemainingBudget();
    return result;
}

ExpenseActionResult ExpenseController::addExpense(const QString& amountText, const QDate& date,
                                                  const QString& description) {
    ExpenseActionResult result = newResult();
    if (!budget.hasTerm()) {
        result.error = ExpenseActionError::NoTerm;
        return result;
    }
    Expense expense;
    if (!Money::parse(amountText, expense.amount)) {
        result.error = ExpenseActionError::BadAmountText;
        return result;
    }
    // Purchase ids are unique across every term, so the next one comes from the database.
    expense.id = repository.nextExpenseId();
    expense.date = date;
    expense.description = description;

    const Budget before = budget;
    const ExpenseError error = budget.addExpense(expense);
    if (error != ExpenseError::None) {
        result.error = toActionError(error);
        return result;
    }
    const Expense stored = *budget.findExpense(expense.id);
    if (!repository.insertExpense(budget.getBudgetId(), stored)) {
        budget = before;  // keep the screen and the database telling the same story
        result.error = ExpenseActionError::SaveFailed;
        result.saveError = repository.lastError();
        return result;
    }
    result = newResult();
    result.ok = true;
    result.action = ExpenseAction::Added;
    result.expense = stored;
    return result;
}

ExpenseActionResult ExpenseController::updateExpense(int expenseId, const QString& amountText,
                                                     const QDate& date, const QString& description) {
    ExpenseActionResult result = newResult();
    std::optional<Expense> changed = budget.findExpense(expenseId);
    if (!changed) {
        result.error = ExpenseActionError::NotFound;
        return result;
    }
    if (!Money::parse(amountText, changed->amount)) {
        result.error = ExpenseActionError::BadAmountText;
        return result;
    }
    changed->date = date;
    changed->description = description;

    const Budget before = budget;
    const ExpenseError error = budget.updateExpense(*changed);
    if (error != ExpenseError::None) {
        result.error = toActionError(error);
        return result;
    }
    const Expense stored = *budget.findExpense(expenseId);
    if (!repository.updateExpense(stored)) {
        budget = before;
        result.error = ExpenseActionError::SaveFailed;
        result.saveError = repository.lastError();
        return result;
    }
    result = newResult();
    result.ok = true;
    result.action = ExpenseAction::Updated;
    result.expense = stored;
    return result;
}

ExpenseActionResult ExpenseController::removeExpense(int expenseId) {
    ExpenseActionResult result = newResult();
    const std::optional<Expense> removed = budget.findExpense(expenseId);
    if (!removed) {
        result.error = ExpenseActionError::NotFound;
        return result;
    }
    const Budget before = budget;
    budget.removeExpense(expenseId);
    if (!repository.deleteExpense(expenseId)) {
        budget = before;
        result.error = ExpenseActionError::SaveFailed;
        result.saveError = repository.lastError();
        return result;
    }
    result = newResult();
    result.ok = true;
    result.action = ExpenseAction::Removed;
    result.expense = *removed;
    return result;
}

ExpenseActionResult ExpenseController::undoLastChange() {
    ExpenseActionResult result = newResult();
    const std::optional<ExpenseChange> change = budget.getLastChange();
    if (!change) {
        result.error = ExpenseActionError::NothingToUndo;
        return result;
    }
    const Budget before = budget;
    const ExpenseError error = budget.undoLastChange();
    if (error != ExpenseError::None) {
        result.error = toActionError(error);
        return result;
    }

    const Expense& expense = change->before;
    bool saved = false;
    switch (change->type) {
    case ExpenseChange::Type::Added:
        saved = repository.deleteExpense(expense.id);
        break;
    case ExpenseChange::Type::Edited:
        saved = repository.updateExpense(expense);
        break;
    case ExpenseChange::Type::Removed:
        saved = repository.insertExpense(budget.getBudgetId(), expense);
        break;
    }
    if (!saved) {
        budget = before;
        result.error = ExpenseActionError::SaveFailed;
        result.saveError = repository.lastError();
        return result;
    }
    result = newResult();
    result.ok = true;
    result.action = ExpenseAction::Undone;
    result.undoneChange = change->type;
    result.expense = expense;
    return result;
}

bool ExpenseController::canUndo() const {
    return budget.getLastChange().has_value();
}

std::optional<ExpenseChange::Type> ExpenseController::lastChangeType() const {
    const std::optional<ExpenseChange> change = budget.getLastChange();
    if (!change) {
        return std::nullopt;
    }
    return change->type;
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
