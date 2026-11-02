#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QWidget>

class QTabWidget;
class ExpenseController;
class ExpensesView;
class StatusController;
class StatusView;
class TermController;
class TermSetupView;

// The app window: a title at the top and one tab per page. When any page
// changes data, every page redraws so the numbers always agree.
class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(TermController& termController, ExpenseController& expenseController,
               StatusController& statusController, bool hasTerm, QWidget* parent = nullptr);

private slots:
    void refreshAll();

private:
    QTabWidget* tabs;
    StatusView* statusView;
    ExpensesView* expensesView;
    TermSetupView* termSetupView;
};

#endif
