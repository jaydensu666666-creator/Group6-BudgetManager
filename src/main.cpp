// Entry point for the app. This file must NEVER be listed in the test_build
// target in CMakeLists.txt — doctest generates its own main(), and having
// two main() functions will fail to link ("duplicate main").
//
// main() only wires the pieces together: model (Budget, BudgetRepository),
// controllers (one per feature), and the window that holds the views.

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "Budget.h"
#include "BudgetRepository.h"
#include "ExpenseController.h"
#include "MainWindow.h"
#include "StatusController.h"
#include "TermController.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Group6BudgetManager");

    // The database lives in the user's app data folder, e.g.
    // %APPDATA%/Group6BudgetManager on Windows, so it survives rebuilds.
    const QString dataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataFolder);
    BudgetRepository repository(dataFolder + "/budget.db");
    if (!repository.isOpen()) {
        QMessageBox::critical(nullptr, "Budget Manager",
                              "Couldn't open the budget database: " + repository.lastError());
        return 1;
    }

    Budget budget = repository.load().value_or(Budget());
    TermController termController(budget, repository);
    ExpenseController expenseController(budget, repository);
    StatusController statusController(budget);

    MainWindow window(termController, expenseController, statusController, budget.hasTerm());
    window.show();
    return app.exec();
}
