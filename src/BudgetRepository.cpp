#include "BudgetRepository.h"

#include <atomic>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "Money.h"

namespace {

// Qt keeps database connections by name, so every repository gets its own.
QString newConnectionName() {
    static std::atomic<int> counter{0};
    return QString("budget_db_%1").arg(++counter);
}

const char* DATE_FORMAT = "yyyy-MM-dd";

}  // namespace

BudgetRepository::BudgetRepository(const QString& databasePath)
    : connectionName(newConnectionName()) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(databasePath);
    if (!db.open()) {
        error = db.lastError().text();
        return;
    }
    tablesReady = createTables();
}

BudgetRepository::~BudgetRepository() {
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName, false);
        db.close();
    }
    // Must run after every QSqlDatabase handle above is gone.
    QSqlDatabase::removeDatabase(connectionName);
}

bool BudgetRepository::isOpen() const {
    return tablesReady && QSqlDatabase::database(connectionName, false).isOpen();
}

bool BudgetRepository::run(QSqlQuery& query) {
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

bool BudgetRepository::runStatement(const QString& sql) {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    // Keep the first error (e.g. "file is not a database") instead of a later, vaguer one.
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    return run(query);
}

bool BudgetRepository::createTables() {
    return runStatement("PRAGMA foreign_keys = ON") &&
           runStatement(
               "CREATE TABLE IF NOT EXISTS budget ("
               " budget_id INTEGER PRIMARY KEY,"
               " total_amount REAL NOT NULL,"
               " start_date TEXT NOT NULL,"
               " end_date TEXT NOT NULL,"
               " savings_amount REAL NOT NULL)") &&
           runStatement(
               "CREATE TABLE IF NOT EXISTS expense ("
               " expense_id INTEGER PRIMARY KEY,"
               " budget_id INTEGER NOT NULL REFERENCES budget(budget_id),"
               " amount REAL NOT NULL,"
               " date TEXT NOT NULL,"
               " description TEXT NOT NULL)");
}

std::optional<Budget> BudgetRepository::load() {
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery termQuery(db);
    termQuery.prepare(
        "SELECT total_amount, savings_amount, start_date, end_date FROM budget WHERE budget_id = ?");
    termQuery.addBindValue(CURRENT_BUDGET_ID);
    if (!run(termQuery) || !termQuery.next()) {
        return std::nullopt;
    }
    Budget budget;
    budget.setTerm(termQuery.value(0).toDouble(), termQuery.value(1).toDouble(),
                   QDate::fromString(termQuery.value(2).toString(), DATE_FORMAT),
                   QDate::fromString(termQuery.value(3).toString(), DATE_FORMAT));

    QSqlQuery expenseQuery(db);
    expenseQuery.prepare(
        "SELECT expense_id, amount, date, description FROM expense WHERE budget_id = ? ORDER BY expense_id");
    expenseQuery.addBindValue(CURRENT_BUDGET_ID);
    if (!run(expenseQuery)) {
        return std::nullopt;
    }
    while (expenseQuery.next()) {
        Expense expense;
        expense.id = expenseQuery.value(0).toInt();
        expense.amount = expenseQuery.value(1).toDouble();
        expense.date = QDate::fromString(expenseQuery.value(2).toString(), DATE_FORMAT);
        expense.description = expenseQuery.value(3).toString();
        budget.loadExpense(expense);
    }
    return budget;
}

bool BudgetRepository::saveTerm(const Budget& budget) {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare(
        "INSERT INTO budget (budget_id, total_amount, start_date, end_date, savings_amount)"
        " VALUES (?, ?, ?, ?, ?)"
        " ON CONFLICT(budget_id) DO UPDATE SET total_amount = excluded.total_amount,"
        " start_date = excluded.start_date, end_date = excluded.end_date,"
        " savings_amount = excluded.savings_amount");
    query.addBindValue(CURRENT_BUDGET_ID);
    query.addBindValue(budget.getReceivedAmount());
    query.addBindValue(budget.getStartDate().toString(DATE_FORMAT));
    query.addBindValue(budget.getNextTransferDate().toString(DATE_FORMAT));
    query.addBindValue(budget.getSavingsAmount());
    return run(query);
}

bool BudgetRepository::insertExpense(const Expense& expense) {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare(
        "INSERT INTO expense (expense_id, budget_id, amount, date, description) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(expense.id);
    query.addBindValue(CURRENT_BUDGET_ID);
    query.addBindValue(expense.amount);
    query.addBindValue(expense.date.toString(DATE_FORMAT));
    query.addBindValue(expense.description);
    return run(query);
}

bool BudgetRepository::updateExpense(const Expense& expense) {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare("UPDATE expense SET amount = ?, date = ?, description = ? WHERE expense_id = ?");
    query.addBindValue(expense.amount);
    query.addBindValue(expense.date.toString(DATE_FORMAT));
    query.addBindValue(expense.description);
    query.addBindValue(expense.id);
    if (!run(query)) {
        return false;
    }
    if (query.numRowsAffected() != 1) {
        error = "That expense is no longer in the database.";
        return false;
    }
    return true;
}

bool BudgetRepository::deleteExpense(int expenseId) {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare("DELETE FROM expense WHERE expense_id = ?");
    query.addBindValue(expenseId);
    if (!run(query)) {
        return false;
    }
    if (query.numRowsAffected() != 1) {
        error = "That expense is no longer in the database.";
        return false;
    }
    return true;
}

double BudgetRepository::totalSpent() {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    // ISO dates (yyyy-MM-dd) compare correctly as text. Like Budget, count only the
    // expenses inside the term: from the start date up to the day before the transfer.
    query.prepare(
        "SELECT COALESCE(SUM(e.amount), 0) FROM expense e JOIN budget b ON b.budget_id = e.budget_id"
        " WHERE e.budget_id = ? AND e.date >= b.start_date AND e.date < b.end_date");
    query.addBindValue(CURRENT_BUDGET_ID);
    if (!run(query) || !query.next()) {
        return 0.0;
    }
    return Money::roundToCents(query.value(0).toDouble());
}
