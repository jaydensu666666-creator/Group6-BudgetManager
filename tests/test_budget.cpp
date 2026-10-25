#include "doctest.h"
#include "Budget.h"

// Author: Jayden Su
// Tests that recording an expense updates the budget.

TEST_CASE("Adding an expense updates remaining budget") {
    Budget budget(500.0);

    bool result = budget.addExpense(20.0);

    CHECK(result == true);
    CHECK(budget.getTotalExpenses() == 20.0);
    CHECK(budget.getRemainingBudget() == 480.0);
}