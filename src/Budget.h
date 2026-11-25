#ifndef BUDGET_H
#define BUDGET_H

#include <optional>
#include <vector>

#include <QDate>
#include <QString>

#include "Expense.h"

// Why a term setup was rejected. The controller turns each one into a message.
enum class TermError {
    None,
    ReceivedNotPositive,
    AmountTooLarge,
    SavingsNegative,
    SavingsMoreThanReceived,
    MissingDate,
    TransferNotAfterStart,
    ExpensesOutsideTerm,
};

// Why an expense change was rejected (S-6 in SPECIFICATIONS.md lists the rules).
enum class ExpenseError {
    None,
    AmountNotPositive,
    AmountTooLarge,
    InvalidDate,
    BeforeTermStart,
    OnOrAfterNextTransfer,
    DescriptionTooLong,
    DuplicateId,
    NotFound,
    NothingToUndo,
};

// The last add, correction or removal, kept so it can be undone (UR-7).
struct ExpenseChange {
    enum class Type { Added, Edited, Removed };
    Type type;
    Expense before;  // the expense as it was before the change (the new one for Added)
};

// The money for one term: what the student received, what they set aside for
// saving, when the next transfer comes, and the expenses recorded in it.
// This is the Budget entity in our ER diagram (Term + Transfer in the P4 CRC).
//
// One Budget is one term: a Budget object holds this term's expenses and
// nothing else, and the database keeps one budget row per term (M2 feedback).
// A purchase belongs to its term by budget_id, never by its date, so moving a
// term's dates can not pull another term's purchases into it, and starting a
// new term leaves the old purchases and the old undo step behind.
// Totals are calculated from the list every time instead of being stored, so an
// edited or removed expense can never leave an old total behind (P5 feedback).
// Methods that depend on the date take "today" as a parameter so tests can fix it.
class Budget {
public:
    static constexpr int MAX_DESCRIPTION_LENGTH = 100;

    // A budget with no term set up yet.
    Budget() = default;

    // Walking-skeleton constructor: a term that starts today with no savings and
    // no next transfer date yet. Kept so the M1 tests still run unchanged.
    explicit Budget(double receivedAmount);

    // Planning the term
    // A start date on or after the current next transfer date starts the next
    // term: this term's expenses and its undo step are left with the old term.
    TermError setTerm(double receivedAmount, double savingsAmount,
                      const QDate& startDate, const QDate& nextTransferDate);
    bool hasTerm() const;
    // The row in the budget table this term is saved in, or 0 when it has not
    // been saved yet (a term that has just been started).
    int getBudgetId() const { return budgetId; }
    void setBudgetId(int id) { budgetId = id; }
    double getReceivedAmount() const { return receivedAmount; }
    double getSavingsAmount() const { return savingsAmount; }
    QDate getStartDate() const { return startDate; }
    QDate getNextTransferDate() const { return nextTransferDate; }

    // Recording money in and out
    ExpenseError checkExpense(double amount, const QDate& date, const QString& description) const;
    ExpenseError addExpense(const Expense& expense);
    bool addExpense(double amount);  // walking-skeleton shortcut, dated on the start date
    ExpenseError updateExpense(const Expense& changed);
    bool removeExpense(int expenseId);
    std::optional<ExpenseChange> getLastChange() const { return lastChange; }
    // Reverses the last add, correction or removal (one step). An expense that no
    // longer fits the term is not put back; the change is then forgotten.
    ExpenseError undoLastChange();
    // Puts back an expense read from the database without checking it again,
    // so a saved expense never disappears just because the term was edited.
    void loadExpense(const Expense& expense);
    std::optional<Expense> findExpense(int expenseId) const;
    const std::vector<Expense>& getExpenses() const { return expenses; }  // this term
    std::vector<Expense> termExpensesNewestFirst() const;
    // Highest id in this term plus one. Purchase ids have to be unique across
    // every term, so the app asks BudgetRepository for the id of a new purchase;
    // this one is for a model used on its own (the walking skeleton and the tests).
    int nextExpenseId() const;
    // The date a new expense starts on: today, or the start date if the term has
    // not started yet. After the term ends it stays today, so the user is told to
    // start the next term instead of a purchase landing in the old one.
    QDate suggestedExpenseDate(const QDate& today) const;

    // Checking where they stand
    double getSpendableAmount() const;  // received minus savings
    double getTotalExpenses() const;
    double getRemainingBudget() const;  // money left to spend this term
    bool isTermOver(const QDate& today) const;
    int daysUntilNextTransfer(const QDate& today) const;
    double spendingPerDay(const QDate& today) const;
    double projectedSpending(const QDate& today) const;
    bool willLastUntilNextTransfer(const QDate& today) const;
    QDate runOutDate(const QDate& today) const;  // invalid date when the money lasts
    QDate weekStart(const QDate& today) const;
    double weeklyAllowance(const QDate& today) const;
    double spentThisWeek(const QDate& today) const;
    double leftToSpendThisWeek(const QDate& today) const;

private:
    int budgetId = 0;
    double receivedAmount = 0.0;
    double savingsAmount = 0.0;
    QDate startDate;
    QDate nextTransferDate;
    std::vector<Expense> expenses;
    std::optional<ExpenseChange> lastChange;

    // Sum of expenses dated from `from` up to but not including `before`.
    // An invalid date means "no limit" on that side.
    double totalBetween(const QDate& from, const QDate& before) const;
    std::vector<Expense>::iterator findById(int expenseId);
    static bool fitsTerm(const QDate& date, const QDate& start, const QDate& nextTransfer);
};

#endif
