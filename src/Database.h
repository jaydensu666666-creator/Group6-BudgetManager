#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

// Opens the SQLite file and creates every table in the ER diagram
// (docs/ER_diagram.drawio.png) in one place, so the repositories never create
// or change a table themselves and nobody has to edit the table setup when a
// new feature arrives (final project plan: the foundation).
//
// Only the storage classes run queries: they ask this class for the connection.
// A database file from the M2 version still opens: the columns and tables added
// for the final project are added to it in place, with the saved data kept.
class Database {
public:
    // Pass ":memory:" for a throwaway database (used by the tests).
    explicit Database(const QString& filePath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool isOpen() const;
    QString lastError() const { return error; }

    // The connection the repositories run their queries on.
    QSqlDatabase connection() const;

    // The table names, so tests and the schema check in tests/test_database.cpp
    // compare the real database with the ER diagram in one place.
    static constexpr const char* BUDGET_TABLE = "budget";
    static constexpr const char* EXPENSE_TABLE = "expense";
    static constexpr const char* CATEGORY_TABLE = "category";
    static constexpr const char* BUDGET_CATEGORY_TABLE = "budget_category";
    static constexpr const char* INCOME_TABLE = "income";
    static constexpr const char* SAVINGS_GOAL_TABLE = "savings_goal";

private:
    QString connectionName;
    QString error;
    bool ready = false;  // false when the file opened but the tables could not be made

    bool createTables();
    // Adds the columns and tables the final project needs to a database file
    // written by the M2 version, without losing anything already saved.
    bool migrate();
    bool runStatement(const QString& sql);
    bool hasColumn(const QString& table, const QString& column);
};

#endif
