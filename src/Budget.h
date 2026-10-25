#ifndef BUDGET_H
#define BUDGET_H

class Budget {
private:
    double initialBudget;
    double totalExpenses;

public:
    Budget(double amount);

    bool addExpense(double amount);
    double getRemainingBudget() const;
    double getTotalExpenses() const;
};

#endif