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
    // One budget row per term (the old MVP reused budget_id 1 for every term,
    // which is what let last term's purchases leak into the new term).
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
    // The term the user is on is the newest one.
    termQuery.prepare(
        "SELECT budget_id, total_amount, savings_amount, start_date, end_date FROM budget"
        " ORDER BY budget_id DESC LIMIT 1");
    if (!run(termQuery) || !termQuery.next()) {
        return std::nullopt;
    }
    Budget budget;
    budget.setBudgetId(termQuery.value(0).toInt());
    budget.setTerm(termQuery.value(1).toDouble(), termQuery.value(2).toDouble(),
                   QDate::fromString(termQuery.value(3).toString(), DATE_FORMAT),
                   QDate::fromString(termQuery.value(4).toString(), DATE_FORMAT));

    // Only this term's purchases: that is what keeps the last term's purchases out
    // of the totals after a new term has started.
    QSqlQuery expenseQuery(db);
    expenseQuery.prepare(
        "SELECT expense_id, amount, date, description FROM expense WHERE budget_id = ? ORDER BY expense_id");
    expenseQuery.addBindValue(budget.getBudgetId());
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

bool BudgetRepository::saveTerm(Budget& budget) {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    if (budget.getBudgetId() == 0) {
        // A term that has just been started gets its own row, so the last term's
        // row and purchases stay with the last term.
        query.prepare(
            "INSERT INTO budget (total_amount, start_date, end_date, savings_amount) VALUES (?, ?, ?, ?)");
        query.addBindValue(budget.getReceivedAmount());
        query.addBindValue(budget.getStartDate().toString(DATE_FORMAT));
        query.addBindValue(budget.getNextTransferDate().toString(DATE_FORMAT));
        query.addBindValue(budget.getSavingsAmount());
        if (!run(query)) {
            return false;
        }
        budget.setBudgetId(query.lastInsertId().toInt());
        return true;
    }

    query.prepare(
        "UPDATE budget SET total_amount = ?, start_date = ?, end_date = ?, savings_amount = ?"
        " WHERE budget_id = ?");
    query.addBindValue(budget.getReceivedAmount());
    query.addBindValue(budget.getStartDate().toString(DATE_FORMAT));
    query.addBindValue(budget.getNextTransferDate().toString(DATE_FORMAT));
    query.addBindValue(budget.getSavingsAmount());
    query.addBindValue(budget.getBudgetId());
    if (!run(query)) {
        return false;
    }
    if (query.numRowsAffected() != 1) {
        error = "That term is no longer in the database.";
        return false;
    }
    return true;
}

bool BudgetRepository::insertExpense(int budgetId, const Expense& expense) {
    if (budgetId <= 0) {
        error = "Save the term before recording a purchase.";
        return false;
    }
    QSqlQuery query(QSqlDatabase::database(connectionName));
    query.prepare(
        "INSERT INTO expense (expense_id, budget_id, amount, date, description) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(expense.id);
    query.addBindValue(budgetId);
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
    // The current term is the newest budget row; its purchases are the whole total,
    // with no date matching (M2 feedback).
    query.prepare(
        "SELECT COALESCE(SUM(amount), 0) FROM expense WHERE budget_id ="
        " (SELECT budget_id FROM budget ORDER BY budget_id DESC LIMIT 1)");
    if (!run(query) || !query.next()) {
        return 0.0;
    }
    return Money::roundToCents(query.value(0).toDouble());
}

int BudgetRepository::nextExpenseId() {
    QSqlQuery query(QSqlDatabase::database(connectionName));
    // Across every term, because expense_id is the primary key of one table.
    query.prepare("SELECT COALESCE(MAX(expense_id), 0) + 1 FROM expense");
    if (!run(query) || !query.next()) {
        return 1;
    }
    return query.value(0).toInt();
}
