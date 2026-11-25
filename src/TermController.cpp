#include "TermController.h"

#include "Budget.h"
#include "BudgetRepository.h"
#include "Money.h"

namespace {

// The model's reasons are part of what the view can show; this keeps the
// controller's result free of any sentence.
TermSaveError toSaveError(TermError error) {
    switch (error) {
    case TermError::ReceivedNotPositive:
        return TermSaveError::ReceivedNotPositive;
    case TermError::AmountTooLarge:
        return TermSaveError::AmountTooLarge;
    case TermError::SavingsNegative:
        return TermSaveError::SavingsNegative;
    case TermError::SavingsMoreThanReceived:
        return TermSaveError::SavingsMoreThanReceived;
    case TermError::MissingDate:
        return TermSaveError::MissingDate;
    case TermError::TransferNotAfterStart:
        return TermSaveError::TransferNotAfterStart;
    case TermError::ExpensesOutsideTerm:
        return TermSaveError::ExpensesOutsideTerm;
    case TermError::None:
        break;
    }
    return TermSaveError::None;
}

}  // namespace

TermController::TermController(Budget& budget, BudgetRepository& repository)
    : budget(budget), repository(repository) {}

TermSaveResult TermController::saveTerm(const QString& receivedText, const QString& savingsText,
                                        const QDate& startDate, const QDate& nextTransferDate) {
    TermSaveResult result;
    double received = 0.0;
    if (!Money::parse(receivedText, received)) {
        result.error = TermSaveError::BadReceivedText;
        return result;
    }
    // Leaving savings empty means the student is not setting anything aside.
    double savings = 0.0;
    if (!savingsText.trimmed().isEmpty() && !Money::parse(savingsText, savings)) {
        result.error = TermSaveError::BadSavingsText;
        return result;
    }

    // A start on or after the next transfer date begins the next term. Only allow that once
    // the transfer has arrived, so this term's expenses are not hidden while it is still running.
    const bool startsNextTerm = budget.hasTerm() && startDate >= budget.getNextTransferDate();
    if (startsNextTerm && !budget.isTermOver(QDate::currentDate())) {
        result.error = TermSaveError::TransferNotArrived;
        result.currentTermEnd = budget.getNextTransferDate().addDays(-1);
        result.nextTransferDate = budget.getNextTransferDate();
        return result;
    }

    const Budget before = budget;
    const TermError error = budget.setTerm(received, savings, startDate, nextTransferDate);
    if (error != TermError::None) {
        result.error = toSaveError(error);
        return result;
    }
    if (!repository.saveTerm(budget)) {
        budget = before;  // keep the screen and the database telling the same story
        result.error = TermSaveError::SaveFailed;
        result.saveError = repository.lastError();
        return result;
    }
    result.ok = true;
    result.startedNewTerm = startsNextTerm;
    result.spendableAmount = budget.getSpendableAmount();
    result.remainingAmount = budget.getRemainingBudget();
    return result;
}

TermInfo TermController::currentTerm(const QDate& today) const {
    TermInfo info;
    info.hasTerm = budget.hasTerm();
    info.termOver = budget.isTermOver(today);
    info.receivedAmount = budget.getReceivedAmount();
    info.savingsAmount = budget.getSavingsAmount();
    info.startDate = budget.getStartDate();
    info.nextTransferDate = budget.getNextTransferDate();
    return info;
}
