#ifndef BUDGET_REPOSITORY_H
#define BUDGET_REPOSITORY_H

#include <optional>

#include <QString>

#include "Budget.h"
#include "Expense.h"

class QSqlQuery;

// Saves the terms and their expenses in SQLite so they are still there after the
// app closes. This is the only class that runs SQL; the tables match the ER
// diagram (docs/ER_diagram.drawio.png).
//
// The budget table has one row per term and every expense row names the term it
// belongs to, so a purchase belongs to its term by budget_id and never by its
// date. That is the fix for the two bugs the instructor found in M2: moving a
// term's dates can not pull another term's purchases in, and a new term can not
// reach the last term's undo step.
class BudgetRepository {
public:
    // Pass ":memory:" for a throwaway database (used by the tests).
    explicit BudgetRepository(const QString& databasePath);
    ~BudgetRepository();

    BudgetRepository(const BudgetRepository&) = delete;
    BudgetRepository& operator=(const BudgetRepository&) = delete;

    bool isOpen() const;
    QString lastError() const { return error; }

    // Returns the term the app is on (the newest budget row) with that term's
    // expenses, or nothing if no term has been set up yet.
    std::optional<Budget> load();
    // Saves the term. A budget with no id yet (a term that has just been started)
    // is added as a new row and gets its id here; a budget that was loaded is
    // updated in place, so the last term's row and purchases stay untouched.
    bool saveTerm(Budget& budget);
    bool insertExpense(int budgetId, const Expense& expense);
    bool updateExpense(const Expense& expense);
    bool deleteExpense(int expenseId);

    // Total spent this term, added up by SQL from the expense rows that belong to
    // the current term. Nothing stores a total (P5 feedback); Budget adds up the
    // same rows for the screen, and the tests check that both give the same answer.
    double totalSpent();
    // The first purchase id that no term uses. Ids are unique across the whole
    // database, so a new term does not take an id that an older term already has.
    int nextExpenseId();

private:
    QString connectionName;
    QString error;
    bool tablesReady = false;  // false for a damaged file that opened but is not a database

    bool createTables();
    bool run(QSqlQuery& query);
    bool runStatement(const QString& sql);
};

#endif
