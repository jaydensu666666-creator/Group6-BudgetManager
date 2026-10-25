// Entry point for the app. This file must NEVER be listed in the test_build
// target in CMakeLists.txt — doctest generates its own main(), and having
// two main() functions will fail to link ("duplicate main").

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleValidator>
#include <QString>

#include "Budget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Create a budget with $500
    Budget budget(500.0);

    // Create the application window
    QWidget window;
    window.setWindowTitle("Group 6 - Budget Manager");
    window.resize(400, 250);

    // Create a vertical layout
    QVBoxLayout *layout = new QVBoxLayout(&window);

    // Create interface elements
    QLabel *title = new QLabel("Budget Manager");
    QLabel *initialLabel = new QLabel("Initial Budget: $500.00");
    QLabel *remainingLabel = new QLabel("Remaining Budget: $500.00");

    QLineEdit *expenseInput = new QLineEdit();
    expenseInput->setPlaceholderText("Enter expense amount");

    // Only allow nonnegative numbers with two decimal places
    expenseInput->setValidator(
        new QDoubleValidator(0.0, 1000000.0, 2, expenseInput)
    );

    QPushButton *addButton = new QPushButton("Add Expense");
    QLabel *messageLabel = new QLabel("");

    // Add interface elements to the layout
    layout->addWidget(title);
    layout->addWidget(initialLabel);
    layout->addWidget(remainingLabel);
    layout->addWidget(expenseInput);
    layout->addWidget(addButton);
    layout->addWidget(messageLabel);

    // Handle the button click
    QObject::connect(addButton, &QPushButton::clicked, [&]() {
        bool ok = false;
        double amount = expenseInput->text().toDouble(&ok);

        if (ok && budget.addExpense(amount)) {
            remainingLabel->setText(
                "Remaining Budget: $" +
                QString::number(budget.getRemainingBudget(), 'f', 2)
            );

            messageLabel->setText("Expense added successfully!");
            expenseInput->clear();
        } else {
            messageLabel->setText("Please enter a valid expense amount.");
        }
    });

    window.show();

    return app.exec();
}

