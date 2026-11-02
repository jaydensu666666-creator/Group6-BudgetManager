#ifndef BUDGET_REPOSITORY_H
#define BUDGET_REPOSITORY_H

#include <optional>

#include <QString>

#include "Budget.h"
#include "Expense.h"

class QSqlQuery;

// Saves the budget and its expenses in SQLite so they are still there after the
// app closes. This is the only class that runs SQL; the tables match the ER
// diagram (docs/ER_diagram.drawio.png).
class BudgetRepository {
public:
    // Pass ":memory:" for a throwaway database (used by the tests).
    explicit BudgetRepository(const QString& databasePath);
    ~BudgetRepository();

    BudgetRepository(const BudgetRepository&) = delete;
    BudgetRepository& operator=(const BudgetRepository&) = delete;

    bool isOpen() const;
    QString lastError() const { return error; }

    // Returns the saved budget, or nothing if no term has been set up yet.
    std::optional<Budget> load();
    bool saveTerm(const Budget& budget);
    bool insertExpense(const Expense& expense);
    bool updateExpense(const Expense& expense);
    bool deleteExpense(int expenseId);

    // Total spent this term, added up by SQL from the expense rows dated inside the
    // saved term. Nothing stores a total (P5 feedback); Budget adds up the same rows
    // for the screen, and the tests check that both give the same answer.
    double totalSpent();

private:
    // MVP keeps one term at a time, so the budget row always has this id.
    static constexpr int CURRENT_BUDGET_ID = 1;

    QString connectionName;
    QString error;
    bool tablesReady = false;  // false for a damaged file that opened but is not a database

    bool createTables();
    bool run(QSqlQuery& query);
    bool runStatement(const QString& sql);
};

#endif
