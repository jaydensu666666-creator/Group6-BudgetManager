#include "Budget.h"

#include <algorithm>
#include <cmath>

#include "Money.h"

namespace {

constexpr int DAYS_PER_WEEK = 7;
// Totals are rounded to cents, so anything under half a cent is rounding noise.
constexpr double CENT_TOLERANCE = 0.005;

bool isPositiveAmount(double amount) {
    return std::isfinite(amount) && amount > 0.0;
}

QDate mondayOf(const QDate& date) {
    return date.addDays(1 - date.dayOfWeek());  // Qt numbers Monday as 1
}

}  // namespace

Budget::Budget(double receivedAmount)
    : receivedAmount(Money::roundToCents(receivedAmount)),
      startDate(QDate::currentDate()) {}

TermError Budget::setTerm(double newReceived, double newSavings,
                          const QDate& newStart, const QDate& newNextTransfer) {
    if (!isPositiveAmount(newReceived)) {
        return TermError::ReceivedNotPositive;
    }
    if (newReceived > Money::MAX_AMOUNT) {
        return TermError::AmountTooLarge;
    }
    if (!std::isfinite(newSavings) || newSavings < 0.0) {
        return TermError::SavingsNegative;
    }
    if (newSavings > newReceived) {
        return TermError::SavingsMoreThanReceived;
    }
    if (!newStart.isValid() || !newNextTransfer.isValid()) {
        return TermError::MissingDate;
    }
    if (newNextTransfer <= newStart) {
        return TermError::TransferNotAfterStart;
    }
    // A start on or after the current next transfer date begins the next term. Its
    // expenses belong to the old term (their own budget_id in the database), so the
    // new term starts empty and with nothing to undo (M2 feedback). Any other change
    // must not silently push this term's expenses out of the dates.
    const bool startingNewTerm = hasTerm() && newStart >= nextTransferDate;
    if (!startingNewTerm) {
        for (const Expense& expense : expenses) {
            if (!fitsTerm(expense.date, newStart, newNextTransfer)) {
                return TermError::ExpensesOutsideTerm;
            }
        }
    }
    if (startingNewTerm) {
        expenses.clear();
        lastChange.reset();
        budgetId = 0;  // the repository saves the next term as a new row
        carryOver = false;  // a new term starts without last term's leftover
    }
    receivedAmount = Money::roundToCents(newReceived);
    savingsAmount = Money::roundToCents(newSavings);
    startDate = newStart;
    nextTransferDate = newNextTransfer;
    return TermError::None;
}

bool Budget::hasTerm() const {
    return receivedAmount > 0.0 && startDate.isValid() && nextTransferDate.isValid();
}

bool Budget::fitsTerm(const QDate& date, const QDate& start, const QDate& nextTransfer) {
    // The day the next transfer arrives already belongs to the next term.
    const bool afterStart = !start.isValid() || date >= start;
    const bool beforeTransfer = !nextTransfer.isValid() || date < nextTransfer;
    return afterStart && beforeTransfer;
}

ExpenseError Budget::checkExpense(double amount, const QDate& date, const QString& description) const {
    if (!isPositiveAmount(amount)) {
        return ExpenseError::AmountNotPositive;
    }
    if (amount > Money::MAX_AMOUNT) {
        return ExpenseError::AmountTooLarge;
    }
    if (!date.isValid()) {
        return ExpenseError::InvalidDate;
    }
    if (startDate.isValid() && date < startDate) {
        return ExpenseError::BeforeTermStart;
    }
    if (nextTransferDate.isValid() && date >= nextTransferDate) {
        return ExpenseError::OnOrAfterNextTransfer;
    }
    if (description.trimmed().length() > MAX_DESCRIPTION_LENGTH) {
        return ExpenseError::DescriptionTooLong;
    }
    return ExpenseError::None;
}

std::vector<Expense>::iterator Budget::findById(int expenseId) {
    return std::find_if(expenses.begin(), expenses.end(),
                        [&](const Expense& e) { return e.id == expenseId; });
}

ExpenseError Budget::addExpense(const Expense& expense) {
    const ExpenseError error = checkExpense(expense.amount, expense.date, expense.description);
    if (error != ExpenseError::None) {
        return error;
    }
    if (expense.id <= 0 || findById(expense.id) != expenses.end()) {
        return ExpenseError::DuplicateId;
    }
    Expense stored = expense;
    stored.amount = Money::roundToCents(expense.amount);
    stored.description = expense.description.trimmed();
    expenses.push_back(stored);
    lastChange = ExpenseChange{ExpenseChange::Type::Added, stored};
    return ExpenseError::None;
}

bool Budget::addExpense(double amount) {
    Expense expense;
    expense.id = nextExpenseId();
    expense.amount = amount;
    expense.date = startDate;
    return addExpense(expense) == ExpenseError::None;
}

ExpenseError Budget::updateExpense(const Expense& changed) {
    auto found = findById(changed.id);
    if (found == expenses.end()) {
        return ExpenseError::NotFound;
    }
    const ExpenseError error = checkExpense(changed.amount, changed.date, changed.description);
    if (error != ExpenseError::None) {
        return error;
    }
    lastChange = ExpenseChange{ExpenseChange::Type::Edited, *found};
    found->amount = Money::roundToCents(changed.amount);
    found->date = changed.date;
    found->description = changed.description.trimmed();
    return ExpenseError::None;
}

bool Budget::removeExpense(int expenseId) {
    auto found = findById(expenseId);
    if (found == expenses.end()) {
        return false;
    }
    lastChange = ExpenseChange{ExpenseChange::Type::Removed, *found};
    expenses.erase(found);
    return true;
}

ExpenseError Budget::undoLastChange() {
    if (!lastChange) {
        return ExpenseError::NothingToUndo;
    }
    const ExpenseChange change = *lastChange;
    lastChange.reset();  // one step of undo: this change is used up either way
    const Expense& before = change.before;
    auto found = findById(before.id);

    switch (change.type) {
    case ExpenseChange::Type::Added:
        if (found == expenses.end()) {
            return ExpenseError::NotFound;
        }
        expenses.erase(found);
        return ExpenseError::None;
    case ExpenseChange::Type::Edited: {
        if (found == expenses.end()) {
            return ExpenseError::NotFound;
        }
        // The term may have changed since, so the old values have to fit it.
        const ExpenseError error = checkExpense(before.amount, before.date, before.description);
        if (error != ExpenseError::None) {
            return error;
        }
        *found = before;
        return ExpenseError::None;
    }
    case ExpenseChange::Type::Removed: {
        if (found != expenses.end()) {
            return ExpenseError::DuplicateId;
        }
        const ExpenseError error = checkExpense(before.amount, before.date, before.description);
        if (error != ExpenseError::None) {
            return error;
        }
        expenses.push_back(before);
        return ExpenseError::None;
    }
    }
    return ExpenseError::None;
}

void Budget::loadExpense(const Expense& expense) {
    expenses.push_back(expense);
}

std::optional<Expense> Budget::findExpense(int expenseId) const {
    for (const Expense& expense : expenses) {
        if (expense.id == expenseId) {
            return expense;
        }
    }
    return std::nullopt;
}

std::vector<Expense> Budget::termExpensesNewestFirst() const {
    std::vector<Expense> inTerm = expenses;
    // Same date: the one recorded later (higher id) comes first.
    std::sort(inTerm.begin(), inTerm.end(), [](const Expense& a, const Expense& b) {
        return a.date != b.date ? a.date > b.date : a.id > b.id;
    });
    return inTerm;
}

int Budget::nextExpenseId() const {
    int highest = 0;
    for (const Expense& expense : expenses) {
        highest = std::max(highest, expense.id);
    }
    return highest + 1;
}

QDate Budget::suggestedExpenseDate(const QDate& today) const {
    if (!hasTerm()) {
        return today;
    }
    if (today < startDate) {
        return startDate;
    }
    return today;
}

double Budget::totalBetween(const QDate& from, const QDate& before) const {
    double total = 0.0;
    for (const Expense& expense : expenses) {
        const bool afterFrom = !from.isValid() || expense.date >= from;
        const bool beforeEnd = !before.isValid() || expense.date < before;
        if (afterFrom && beforeEnd) {
            total += expense.amount;
        }
    }
    return Money::roundToCents(total);
}

double Budget::getSpendableAmount() const {
    return Money::roundToCents(receivedAmount - savingsAmount);
}

double Budget::getTotalExpenses() const {
    // Every expense in a Budget belongs to this term by budget_id, so the total is
    // the whole list; no date matching (M2 feedback: the dates alone let last
    // term's purchases come back into a new term).
    double total = 0.0;
    for (const Expense& expense : expenses) {
        total += expense.amount;
    }
    return Money::roundToCents(total);
}

double Budget::getRemainingBudget() const {
    return Money::roundToCents(getSpendableAmount() - getTotalExpenses());
}

bool Budget::isTermOver(const QDate& today) const {
    return hasTerm() && today >= nextTransferDate;
}

int Budget::daysUntilNextTransfer(const QDate& today) const {
    if (!hasTerm()) {
        return 0;
    }
    return std::max(0, static_cast<int>(today.daysTo(nextTransferDate)));
}

double Budget::spendingPerDay(const QDate& today) const {
    if (!hasTerm() || today < startDate) {
        return 0.0;
    }
    // Count today as a day already spent in, and stop at the last day of the term.
    const QDate lastCountedDay = std::min(today, nextTransferDate.addDays(-1));
    const qint64 daysSoFar = startDate.daysTo(lastCountedDay) + 1;
    const double spentSoFar = totalBetween(startDate, lastCountedDay.addDays(1));
    return spentSoFar / static_cast<double>(daysSoFar);
}

double Budget::projectedSpending(const QDate& today) const {
    if (!hasTerm()) {
        return getTotalExpenses();
    }
    // Days still to come after today and before the transfer arrives.
    const int daysAhead = std::max(0, daysUntilNextTransfer(today) - 1);
    // Expenses already dated in the future are counted once, as recorded.
    return Money::roundToCents(getTotalExpenses() + spendingPerDay(today) * daysAhead);
}

bool Budget::willLastUntilNextTransfer(const QDate& today) const {
    return projectedSpending(today) <= getSpendableAmount() + CENT_TOLERANCE;
}

QDate Budget::runOutDate(const QDate& today) const {
    if (!hasTerm() || willLastUntilNextTransfer(today)) {
        return QDate();
    }
    const double moneyLeft = getRemainingBudget();
    const double perDay = spendingPerDay(today);
    if (moneyLeft <= 0.0 || perDay <= 0.0) {
        return today;
    }
    const int daysLeft = static_cast<int>(std::ceil(moneyLeft / perDay));
    return today.addDays(daysLeft);
}

QDate Budget::weekStart(const QDate& today) const {
    // Weeks start on Monday. The first week of a term starts on the term's start date instead.
    const QDate monday = mondayOf(today);
    if (startDate.isValid() && monday < startDate) {
        return startDate;
    }
    return monday;
}

double Budget::weeklyAllowance(const QDate& today) const {
    if (!hasTerm()) {
        return 0.0;
    }
    const QDate thisWeek = weekStart(today);
    // Count Monday-to-Sunday weeks, so a term that starts on a Wednesday still has its
    // short first week and a short last week (S-14), not 7-day blocks from the start.
    const qint64 daysFromMonday = mondayOf(thisWeek).daysTo(nextTransferDate);
    const int weeksLeft = std::max(1, static_cast<int>(std::ceil(daysFromMonday / static_cast<double>(DAYS_PER_WEEK))));
    // Only spending before this week shrinks the allowance, so what the user
    // spends during the week shows up as "left this week" going down.
    const double leftAtWeekStart = std::max(0.0, getSpendableAmount() - totalBetween(startDate, thisWeek));
    return Money::roundToCents(leftAtWeekStart / weeksLeft);
}

double Budget::spentThisWeek(const QDate& today) const {
    const QDate nextMonday = mondayOf(today).addDays(DAYS_PER_WEEK);
    const QDate weekEnd = (nextTransferDate.isValid() && nextTransferDate < nextMonday) ? nextTransferDate : nextMonday;
    return totalBetween(weekStart(today), weekEnd);
}

double Budget::leftToSpendThisWeek(const QDate& today) const {
    const double left = weeklyAllowance(today) - spentThisWeek(today);
    // Never promise more than the money actually left in the term.
    return Money::roundToCents(std::min(left, getRemainingBudget()));
}
