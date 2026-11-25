#include "MainWindow.h"

#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>

#include "ExpensesView.h"
#include "StatusView.h"
#include "TermSetupView.h"
#include "ViewStyle.h"

MainWindow::MainWindow(TermController& termController, ExpenseController& expenseController,
                       StatusController& statusController, bool hasTerm, QWidget* parent)
    : QWidget(parent) {
    setWindowTitle("Group 6 - Budget Manager");
    resize(560, 620);

    // Visual hierarchy (M1 feedback): the app name first, then what it is for.
    QLabel* title = new QLabel("Budget Manager");
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    QLabel* subtitle = new QLabel("See how much you can spend until your next transfer.");
    subtitle->setStyleSheet("color: " + ViewStyle::MUTED_COLOR + ";");

    statusView = new StatusView(statusController);
    expensesView = new ExpensesView(expenseController);
    termSetupView = new TermSetupView(termController);

    tabs = new QTabWidget;
    // The tabs follow the order the user works in (M2 feedback).
    tabs->addTab(termSetupView, "Term setup");
    tabs->addTab(expensesView, "Expenses");
    tabs->addTab(statusView, "Where I stand");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(tabs);

    connect(expensesView, &ExpensesView::changed, this, &MainWindow::refreshAll);
    connect(termSetupView, &TermSetupView::changed, this, &MainWindow::refreshAll);
    connect(statusView, &StatusView::setUpTermRequested, this,
            [this]() { tabs->setCurrentWidget(termSetupView); });
    // The home page depends on today's date, so redraw it whenever it is opened
    // (the app may have been left open past midnight or into a new week).
    connect(tabs, &QTabWidget::currentChanged, statusView, &StatusView::refresh);

    // A first-time user has nothing to look at yet, so start them on setup;
    // otherwise open on the page they came to check (S-17).
    if (hasTerm) {
        tabs->setCurrentWidget(statusView);
    } else {
        tabs->setCurrentWidget(termSetupView);
    }
}

void MainWindow::refreshAll() {
    statusView->refresh();
    expensesView->refresh();
    termSetupView->refresh();
}
