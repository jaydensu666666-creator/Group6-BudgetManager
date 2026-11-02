#ifndef EXPENSE_H
#define EXPENSE_H

#include <QDate>
#include <QString>

// One purchase the user recorded. Budget checks the values before an
// Expense is stored, so this is only the data (expense table in the ER diagram).
struct Expense {
    int id = 0;
    double amount = 0.0;
    QDate date;
    QString description;
};

#endif
