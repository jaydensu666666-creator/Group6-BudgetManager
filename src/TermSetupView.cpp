#include "TermSetupView.h"

#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

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
    const ActionResult result = controller.saveTerm(receivedInput->text(), savingsInput->text(),
                                                    startInput->date(), nextTransferInput->date());
    messageLabel->setStyleSheet(ViewStyle::messageStyle(result.ok));
    messageLabel->setText(result.message);
    if (result.ok) {
        emit changed();
    }
}
