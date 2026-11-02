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
// saving, when the next transfer comes, and the expenses recorded.
// This is the Budget entity in our ER diagram (Term + Transfer in the P4 CRC).
//
// The term is the dates from the start date up to the day before the next
// transfer. Totals only count expenses inside those dates and are calculated
// from the list every time instead of being stored, so an edited or removed
// expense can never leave an old total behind (P5 feedback). Expenses from an
// earlier term stay in the list (and the database) but are not counted.
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
    TermError setTerm(double receivedAmount, double savingsAmount,
                      const QDate& startDate, const QDate& nextTransferDate);
    bool hasTerm() const;
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
    const std::vector<Expense>& getExpenses() const { return expenses; }  // every term
    std::vector<Expense> termExpensesNewestFirst() const;                 // this term only
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
    double receivedAmount = 0.0;
    double savingsAmount = 0.0;
    QDate startDate;
    QDate nextTransferDate;
    std::vector<Expense> expenses;
    std::optional<ExpenseChange> lastChange;

    bool isInTerm(const QDate& date) const;
    // Sum of expenses dated from `from` up to but not including `before`.
    // An invalid date means "no limit" on that side.
    double totalBetween(const QDate& from, const QDate& before) const;
    std::vector<Expense>::iterator findById(int expenseId);
    static bool fitsTerm(const QDate& date, const QDate& start, const QDate& nextTransfer);
};

#endif
