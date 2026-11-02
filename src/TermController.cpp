#include "TermController.h"

#include <QLocale>

#include "Budget.h"
#include "BudgetRepository.h"
#include "Money.h"

namespace {

QString shortDate(const QDate& date) {
    return QLocale(QLocale::English).toString(date, "MMM d, yyyy");
}

QString termErrorMessage(TermError error) {
    switch (error) {
    case TermError::ReceivedNotPositive:
        return "Enter how much money you received, more than $0.";
    case TermError::AmountTooLarge:
        return "That amount is too large. Enter up to " + Money::format(Money::MAX_AMOUNT) + ".";
    case TermError::SavingsNegative:
        return "The amount to set aside can't be negative. Enter 0 if you don't want to save.";
    case TermError::SavingsMoreThanReceived:
        return "You can't set aside more than the money you received.";
    case TermError::MissingDate:
        return "Pick both a start date and a next transfer date.";
    case TermError::TransferNotAfterStart:
        return "The next transfer has to come after the start date.";
    case TermError::ExpensesOutsideTerm:
        return "Some of this term's expenses would fall outside these dates. Edit or remove them first. "
               "If your next transfer came early, first set Next transfer arrives to the new start date "
               "and save, then enter the new term.";
    case TermError::None:
        break;
    }
    return QString();
}

}  // namespace

TermController::TermController(Budget& budget, BudgetRepository& repository)
    : budget(budget), repository(repository) {}

ActionResult TermController::saveTerm(const QString& receivedText, const QString& savingsText,
                                      const QDate& startDate, const QDate& nextTransferDate) {
    double received = 0.0;
    if (!Money::parse(receivedText, received)) {
        return ActionResult::failure("Enter how much money you received, like 1500 or 1500.00.");
    }
    // Leaving savings empty means the student is not setting anything aside.
    double savings = 0.0;
    if (!savingsText.trimmed().isEmpty() && !Money::parse(savingsText, savings)) {
        return ActionResult::failure("Enter the amount to set aside as a number, like 200.");
    }

    // A start on or after the next transfer date begins the next term. Only allow that once
    // the transfer has arrived, so this term's expenses are not hidden while it is still running.
    const bool startsNextTerm = budget.hasTerm() && startDate >= budget.getNextTransferDate();
    if (startsNextTerm && !budget.isTermOver(QDate::currentDate())) {
        return ActionResult::failure("Your current term runs until " +
                                     shortDate(budget.getNextTransferDate().addDays(-1)) +
                                     ". You can start the next term once your transfer arrives on " +
                                     shortDate(budget.getNextTransferDate()) + ".");
    }

    const Budget before = budget;
    const TermError error = budget.setTerm(received, savings, startDate, nextTransferDate);
    if (error != TermError::None) {
        return ActionResult::failure(termErrorMessage(error));
    }
    if (!repository.saveTerm(budget)) {
        budget = before;  // keep the screen and the database telling the same story
        return ActionResult::failure("Couldn't save your term: " + repository.lastError());
    }
    if (startsNextTerm) {
        return ActionResult::success("New term started. You can spend " +
                                     Money::format(budget.getSpendableAmount()) +
                                     " this term. Last term's expenses are kept but no longer listed.");
    }
    return ActionResult::success("Term saved. You can spend " + Money::format(budget.getSpendableAmount()) +
                                 " this term, and " + Money::format(budget.getRemainingBudget()) +
                                 " of it is left.");
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
