#include "Budget.h"
#include <cmath>

Budget::Budget(double amount) {
    initialBudget = amount;
    totalExpenses = 0;
}

bool Budget::addExpense(double amount) {
    if (!std::isfinite(amount) || amount <= 0) {
        return false;
    }

    totalExpenses += amount;
    return true;
}

double Budget::getRemainingBudget() const {
    return initialBudget - totalExpenses;
}

double Budget::getTotalExpenses() const {
    return totalExpenses;
}