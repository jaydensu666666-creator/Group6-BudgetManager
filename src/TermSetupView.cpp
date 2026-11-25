#include "TermSetupView.h"

#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "Money.h"
#include "TermController.h"
#include "ViewStyle.h"

namespace {

QDateEdit* makeDateInput(const QString& objectName) {
    QDateEdit* input = new QDateEdit;
    input->setObjectName(objectName);
    input->setCalendarPopup(true);
    input->setDisplayFormat(ViewStyle::DATE_FORMAT);
    return input;
}

QLineEdit* makeMoneyInput(const QString& objectName, const QString& placeholder) {
    // No input validator: it would silently drop "-" or "$". The controller
    // reads the text and explains what is wrong instead.
    QLineEdit* input = new QLineEdit;
    input->setObjectName(objectName);
    input->setPlaceholderText(placeholder);
    input->setMaxLength(ViewStyle::MONEY_TEXT_MAX_LENGTH);
    return input;
}

QString shortDate(const QDate& date) {
    return QLocale(QLocale::English).toString(date, "MMM d, yyyy");
}

// The page writes every sentence the user reads; the controller only says what
// happened (M2 feedback: messages do not belong in the controllers).
QString messageFor(const TermSaveResult& result) {
    if (result.ok) {
        if (result.startedNewTerm) {
            return "New term started. You can spend " + Money::format(result.spendableAmount) +
                   " this term. Last term's expenses are kept but no longer listed.";
        }
        return "Term saved. You can spend " + Money::format(result.spendableAmount) + " this term, and " +
               Money::format(result.remainingAmount) + " of it is left.";
    }
    switch (result.error) {
    case TermSaveError::BadReceivedText:
        return "Enter how much money you received, like 1500 or 1500.00.";
    case TermSaveError::BadSavingsText:
        return "Enter the amount to set aside as a number, like 200.";
    case TermSaveError::TransferNotArrived:
        return "Your current term runs until " + shortDate(result.currentTermEnd) +
               ". You can start the next term once your transfer arrives on " +
               shortDate(result.nextTransferDate) + ".";
    case TermSaveError::ReceivedNotPositive:
        return "Enter how much money you received, more than $0.";
    case TermSaveError::AmountTooLarge:
        return "That amount is too large. Enter up to " + Money::format(Money::MAX_AMOUNT) + ".";
    case TermSaveError::SavingsNegative:
        return "The amount to set aside can't be negative. Enter 0 if you don't want to save.";
    case TermSaveError::SavingsMoreThanReceived:
        return "You can't set aside more than the money you received.";
    case TermSaveError::MissingDate:
        return "Pick both a start date and a next transfer date.";
    case TermSaveError::TransferNotAfterStart:
        return "The next transfer has to come after the start date.";
    case TermSaveError::ExpensesOutsideTerm:
        return "Some of this term's expenses would fall outside these dates. Edit or remove them first. "
               "If your next transfer came early, first set Next transfer arrives to the new start date "
               "and save, then enter the new term.";
    case TermSaveError::SaveFailed:
        return "Couldn't save your term: " + result.saveError;
    case TermSaveError::None:
        break;
    }
    return QString();
}

}  // namespace

TermSetupView::TermSetupView(TermController& controller, QWidget* parent)
    : QWidget(parent), controller(controller) {
    receivedInput = makeMoneyInput("receivedInput", "1500.00");
    savingsInput = makeMoneyInput("savingsInput", "0");
    startInput = makeDateInput("startInput");
    nextTransferInput = makeDateInput("nextTransferInput");

    QLabel* savingsHint = new QLabel("Money you set aside is not counted as money to spend.");
    savingsHint->setStyleSheet("color: " + ViewStyle::MUTED_COLOR + ";");

    QFormLayout* form = new QFormLayout;
    form->addRow("Money you received ($)", receivedInput);
    form->addRow("Set aside for saving ($)", savingsInput);
    form->addRow("", savingsHint);
    form->addRow("Term starts", startInput);
    form->addRow("Next transfer arrives", nextTransferInput);

    saveButton = new QPushButton("Save term");
    saveButton->setObjectName("saveTermButton");
    saveButton->setDefault(true);
    messageLabel = new QLabel;
    messageLabel->setObjectName("termMessage");
    messageLabel->setWordWrap(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(saveButton, 0, Qt::AlignLeft);
    layout->addWidget(messageLabel);
    layout->addStretch();

    connect(saveButton, &QPushButton::clicked, this, &TermSetupView::save);
    refresh();
}

void TermSetupView::refresh() {
    const TermInfo term = controller.currentTerm(QDate::currentDate());
    if (term.termOver) {
        // The next term starts when the transfer arrived; the money is new, so start empty.
        receivedInput->clear();
        savingsInput->clear();
        startInput->setDate(term.nextTransferDate);
        nextTransferInput->setDate(term.nextTransferDate.addMonths(1));
        return;
    }
    if (!term.hasTerm) {
        // Suggest a one-month term; the student changes the dates to match their transfer.
        const QDate today = QDate::currentDate();
        startInput->setDate(today);
        nextTransferInput->setDate(today.addMonths(1));
        return;
    }
    receivedInput->setText(QString::number(term.receivedAmount, 'f', 2));
    savingsInput->setText(QString::number(term.savingsAmount, 'f', 2));
    startInput->setDate(term.startDate);
    nextTransferInput->setDate(term.nextTransferDate);
}

void TermSetupView::save() {
    const TermSaveResult result = controller.saveTerm(receivedInput->text(), savingsInput->text(),
                                                      startInput->date(), nextTransferInput->date());
    messageLabel->setStyleSheet(ViewStyle::messageStyle(result.ok));
    messageLabel->setText(messageFor(result));
    if (result.ok) {
        emit changed();
    }
}
