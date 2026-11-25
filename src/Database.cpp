#include "Database.h"

#include <atomic>

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

// Qt keeps database connections by name, so every Database gets its own.
QString newConnectionName() {
    static std::atomic<int> counter{0};
    return QString("budget_db_%1").arg(++counter);
}

}  // namespace

Database::Database(const QString& filePath) : connectionName(newConnectionName()) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(filePath);
    if (!db.open()) {
        error = db.lastError().text();
        return;
    }
    ready = createTables() && migrate();
}

Database::~Database() {
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName, false);
        db.close();
    }
    // Must run after every QSqlDatabase handle above is gone.
    QSqlDatabase::removeDatabase(connectionName);
}

bool Database::isOpen() const {
    return ready && QSqlDatabase::database(connectionName, false).isOpen();
}

QSqlDatabase Database::connection() const {
    return QSqlDatabase::database(connectionName, false);
}

bool Database::runStatement(const QString& sql) {
    QSqlQuery query(connection());
    // Keep the first error (e.g. "file is not a database") instead of a later, vaguer one.
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

bool Database::createTables() {
    // Every table in the ER diagram, created together so the repositories can
    // assume they exist. Names are frozen: each feature owner builds on them.
    // The one-to-many lines are foreign keys (expense.budget_id, income.budget_id,
    // budget.goal_id, expense.category_id); the many-to-many diamond between
    // Budget and Category is its own table with limit_amount.
    return runStatement("PRAGMA foreign_keys = ON") &&

           runStatement(
               "CREATE TABLE IF NOT EXISTS category ("
               " category_id INTEGER PRIMARY KEY,"
               " name TEXT NOT NULL)") &&

           runStatement(
               "CREATE TABLE IF NOT EXISTS savings_goal ("
               " goal_id INTEGER PRIMARY KEY,"
               " name TEXT NOT NULL,"
               " target_amount REAL NOT NULL)") &&

           // One row per term (M2 feedback). carry_over is the user's yes/no
           // choice to bring over what was left last term; the amount itself is
           // calculated and never stored. goal_id points at the savings goal this
           // term saves toward.
           runStatement(
               "CREATE TABLE IF NOT EXISTS budget ("
               " budget_id INTEGER PRIMARY KEY,"
               " total_amount REAL NOT NULL,"
               " start_date TEXT NOT NULL,"
               " end_date TEXT NOT NULL,"
               " savings_amount REAL NOT NULL,"
               " carry_over INTEGER NOT NULL DEFAULT 0,"
               " goal_id INTEGER REFERENCES savings_goal(goal_id))") &&

           // Each purchase belongs to its term by budget_id (never by its date)
           // and may belong to one category.
           runStatement(
               "CREATE TABLE IF NOT EXISTS expense ("
               " expense_id INTEGER PRIMARY KEY,"
               " budget_id INTEGER NOT NULL REFERENCES budget(budget_id),"
               " amount REAL NOT NULL,"
               " date TEXT NOT NULL,"
               " description TEXT NOT NULL,"
               " category_id INTEGER REFERENCES category(category_id))") &&

           // The many-to-many diamond: one term can limit many categories, and
           // one category can have a different limit in many terms.
           runStatement(
               "CREATE TABLE IF NOT EXISTS budget_category ("
               " budget_id INTEGER NOT NULL REFERENCES budget(budget_id),"
               " category_id INTEGER NOT NULL REFERENCES category(category_id),"
               " limit_amount REAL NOT NULL,"
               " PRIMARY KEY (budget_id, category_id))") &&

           // Extra money outside the regular transfer. to_savings says whether the
           // user put it toward saving instead of spending it.
           runStatement(
               "CREATE TABLE IF NOT EXISTS income ("
               " income_id INTEGER PRIMARY KEY,"
               " budget_id INTEGER NOT NULL REFERENCES budget(budget_id),"
               " amount REAL NOT NULL,"
               " date TEXT NOT NULL,"
               " source TEXT NOT NULL,"
               " to_savings INTEGER NOT NULL DEFAULT 0)");
}

bool Database::hasColumn(const QString& table, const QString& column) {
    QSqlQuery query(connection());
    // The table name is one of our own constants, never text from the user.
    if (!query.exec(QString("PRAGMA table_info(%1)").arg(table))) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) {
        if (query.value(1).toString() == column) {
            return true;
        }
    }
    return false;
}

bool Database::migrate() {
    // The M2 version saved one budget row and expenses without a category. The
    // new columns are added to an existing file here, so the user's saved term
    // and purchases survive the update.
    if (!hasColumn(BUDGET_TABLE, "carry_over") &&
        !runStatement(
            "ALTER TABLE budget ADD COLUMN carry_over INTEGER NOT NULL DEFAULT 0")) {
        return false;
    }
    if (!hasColumn(BUDGET_TABLE, "goal_id") &&
        !runStatement("ALTER TABLE budget ADD COLUMN goal_id INTEGER REFERENCES savings_goal(goal_id)")) {
        return false;
    }
    if (!hasColumn(EXPENSE_TABLE, "category_id") &&
        !runStatement(
            "ALTER TABLE expense ADD COLUMN category_id INTEGER REFERENCES category(category_id)")) {
        return false;
    }
    return true;
}
