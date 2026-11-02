#ifndef EXPENSES_VIEW_H
#define EXPENSES_VIEW_H

#include <QWidget>

class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class ExpenseController;

// Expenses page: add an expense, see the list, correct or remove one, undo.
class ExpensesView : public QWidget {
    Q_OBJECT
public:
    explicit ExpensesView(ExpenseController& controller, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void changed();

private slots:
    void submitForm();
    void startEditingSelected();
    void cancelEditing();
    void removeSelected();
    void undo();

private:
    // 0 means the form adds a new expense; otherwise it is the id being edited.
    static constexpr int NOT_EDITING = 0;

    ExpenseController& controller;
    int editingId = NOT_EDITING;

    QLineEdit* amountInput;
    QDateEdit* dateInput;
    QLineEdit* descriptionInput;
    QPushButton* submitButton;
    QPushButton* cancelButton;
    QTableWidget* table;
    QPushButton* editButton;
    QPushButton* removeButton;
    QPushButton* undoButton;
    QLabel* messageLabel;

    int selectedExpenseId() const;
    void clearForm();
    void showMessage(bool ok, const QString& text);
};

#endif
