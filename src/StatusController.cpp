#include "StatusController.h"

#include "Budget.h"

StatusController::StatusController(const Budget& budget) : budget(budget) {}

SpendingStatus StatusController::status(const QDate& today) const {
    SpendingStatus status;
    status.hasTerm = budget.hasTerm();
    if (!status.hasTerm) {
        return status;
    }
    status.termOver = budget.isTermOver(today);
    status.receivedAmount = budget.getReceivedAmount();
    status.savingsAmount = budget.getSavingsAmount();
    status.spendableAmount = budget.getSpendableAmount();
    status.totalSpent = budget.getTotalExpenses();
    status.moneyLeft = budget.getRemainingBudget();
    status.nextTransferDate = budget.getNextTransferDate();
    status.daysUntilNextTransfer = budget.daysUntilNextTransfer(today);
    status.spendingPerDay = budget.spendingPerDay(today);
    status.willLast = budget.willLastUntilNextTransfer(today);
    status.runOutDate = budget.runOutDate(today);
    status.weeklyAllowance = budget.weeklyAllowance(today);
    status.spentThisWeek = budget.spentThisWeek(today);
    status.leftThisWeek = budget.leftToSpendThisWeek(today);
    return status;
}
