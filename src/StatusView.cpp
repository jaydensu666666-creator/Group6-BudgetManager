#include "StatusView.h"

#include <QDate>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

#include "Money.h"
#include "StatusController.h"
#include "ViewStyle.h"

namespace {

QString monthDay(const QDate& date) {
    return QLocale(QLocale::English).toString(date, "MMM d");
}

QString days(int count) {
    return QString("%1 day%2").arg(count).arg(count == 1 ? "" : "s");
}

QString colored(const QString& color, int fontSize, bool bold = false) {
    return QString("color: %1; font-size: %2px;%3").arg(color).arg(fontSize).arg(bold ? " font-weight: bold;" : "");
}

}  // namespace

StatusView::StatusView(StatusController& controller, QWidget* parent)
    : QWidget(parent), controller(controller) {
    QLabel* caption = new QLabel("Money left to spend");
    caption->setStyleSheet(colored(ViewStyle::MUTED_COLOR, 14));

    moneyLeftLabel = new QLabel;
    moneyLeftLabel->setObjectName("moneyLeft");
    spendableLabel = new QLabel;
    spendableLabel->setStyleSheet(colored(ViewStyle::MUTED_COLOR, 13));

    weekLabel = new QLabel;
    weekLabel->setObjectName("weekLeft");
    weekLabel->setStyleSheet("font-size: 16px; font-weight: 600;");
    weekLabel->setWordWrap(true);

    paceLabel = new QLabel;
    paceLabel->setObjectName("pace");
    paceLabel->setWordWrap(true);

    detailsLabel = new QLabel;
    detailsLabel->setObjectName("details");
    detailsLabel->setStyleSheet(colored(ViewStyle::MUTED_COLOR, 13));

    setUpButton = new QPushButton("Set up your term");
    connect(setUpButton, &QPushButton::clicked, this, &StatusView::setUpTermRequested);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(caption);
    layout->addWidget(moneyLeftLabel);
    layout->addWidget(spendableLabel);
    layout->addSpacing(12);
    layout->addWidget(weekLabel);
    layout->addWidget(paceLabel);
    layout->addSpacing(12);
    layout->addWidget(detailsLabel);
    layout->addWidget(setUpButton, 0, Qt::AlignLeft);
    layout->addStretch();

    refresh();
}

void StatusView::refresh() {
    const SpendingStatus status = controller.status(QDate::currentDate());
    setUpButton->setVisible(!status.hasTerm || status.termOver);

    if (!status.hasTerm) {
        moneyLeftLabel->setText("--");
        moneyLeftLabel->setStyleSheet(colored(ViewStyle::MUTED_COLOR, 36, true));
        spendableLabel->setText("Set up your term to see how much you can spend.");
        setUpButton->setText("Set up your term");
        weekLabel->clear();
        paceLabel->clear();
        detailsLabel->clear();
        return;
    }

    const bool overspent = status.moneyLeft < 0.0;
    moneyLeftLabel->setText(Money::format(status.moneyLeft));
    moneyLeftLabel->setStyleSheet(colored(overspent ? ViewStyle::ERROR_COLOR : ViewStyle::OK_COLOR, 36, true));
    spendableLabel->setText("out of " + Money::format(status.spendableAmount) + " you can spend this term");

    // Show the working, not only the answer (M2 feedback): the user asked how much
    // is left, and this says how the app got there.
    QString details = QString("Money received: %1\nMinus set aside for saving: %2\nMinus spent: %3\n= Money left to spend: %4")
                          .arg(Money::format(status.receivedAmount), Money::format(status.savingsAmount),
                               Money::format(status.totalSpent), Money::format(status.moneyLeft));

    if (status.termOver) {
        setUpButton->setText("Start your next term");
        weekLabel->setText("This term ended when your next transfer was due on " +
                           monthDay(status.nextTransferDate) + ".");
        paceLabel->setText("To start the next term, open Term setup. The start date is already set to " +
                           monthDay(status.nextTransferDate) + "; enter the new money and press Save term.");
        paceLabel->setStyleSheet(colored(ViewStyle::MUTED_COLOR, 14));
        detailsLabel->setText(details);
        return;
    }

    if (overspent) {
        weekLabel->setText("You have spent " + Money::format(-status.moneyLeft) +
                           " more than this term's money, so there is nothing left for this week.");
    } else if (status.leftThisWeek >= 0.0) {
        weekLabel->setText("You can still spend " + Money::format(status.leftThisWeek) + " this week.");
    } else {
        weekLabel->setText("You are " + Money::format(-status.leftThisWeek) + " over this week's amount.");
    }

    const QString transferDay = monthDay(status.nextTransferDate) + " (" + days(status.daysUntilNextTransfer) + " away)";
    if (status.willLast) {
        paceLabel->setText("At your current pace, your money will last until your next transfer on " + transferDay + ".");
        paceLabel->setStyleSheet(colored(ViewStyle::OK_COLOR, 14));
    } else if (overspent || status.moneyLeft == 0.0) {
        paceLabel->setText("This term's money is already used up. Your next transfer is on " + transferDay + ".");
        paceLabel->setStyleSheet(colored(ViewStyle::ERROR_COLOR, 14));
    } else {
        paceLabel->setText("At your current pace, you will run out around " + monthDay(status.runOutDate) +
                           ", before your next transfer on " + transferDay + ".");
        paceLabel->setStyleSheet(colored(ViewStyle::ERROR_COLOR, 14));
    }

    details += QString("\nThis week's amount: %1 (spent this week: %2)\nAverage spending per day: %3")
                   .arg(Money::format(status.weeklyAllowance), Money::format(status.spentThisWeek),
                        Money::format(status.spendingPerDay));
    detailsLabel->setText(details);
}
