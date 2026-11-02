#include "ExpensesView.h"

#include <QDateEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "ExpenseController.h"
#include "Money.h"
#include "ViewStyle.h"

namespace {

enum Column { DATE_COLUMN, DESCRIPTION_COLUMN, AMOUNT_COLUMN, COLUMN_COUNT };

}  // namespace

ExpensesView::ExpensesView(ExpenseController& controller, QWidget* parent)
    : QWidget(parent), controller(controller) {
    // No input validator on purpose: a validator silently drops keys like "-" or "$",
    // so "-5" would turn into 5. The controller reads the text and says what is wrong.
    amountInput = new QLineEdit;
    amountInput->setObjectName("amountInput");
    amountInput->setPlaceholderText("12.50");
    amountInput->setMaxLength(ViewStyle::MONEY_TEXT_MAX_LENGTH);

    dateInput = new QDateEdit;
    dateInput->setObjectName("dateInput");
    dateInput->setCalendarPopup(true);
    dateInput->setDisplayFormat(ViewStyle::DATE_FORMAT);

    descriptionInput = new QLineEdit;
    descriptionInput->setObjectName("descriptionInput");
    descriptionInput->setPlaceholderText("What was it for? (optional)");
    descriptionInput->setMaxLength(ExpenseController::MAX_DESCRIPTION_LENGTH);

    submitButton = new QPushButton("Add expense");
    submitButton->setObjectName("submitButton");
    submitButton->setDefault(true);
    cancelButton = new QPushButton("Cancel edit");
    cancelButton->setVisible(false);

    QFormLayout* form = new QFormLayout;
    form->addRow("Amount ($)", amountInput);
    form->addRow("Date", dateInput);
    form->addRow("Description", descriptionInput);

    QHBoxLayout* formButtons = new QHBoxLayout;
    formButtons->addWidget(submitButton);
    formButtons->addWidget(cancelButton);
    formButtons->addStretch();

    messageLabel = new QLabel;
    messageLabel->setObjectName("expenseMessage");
    messageLabel->setWordWrap(true);

    table = new QTableWidget(0, COLUMN_COUNT);
    table->setObjectName("expenseTable");
    table->setHorizontalHeaderLabels({"Date", "Description", "Amount"});
    table->horizontalHeader()->setSectionResizeMode(DATE_COLUMN, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(DESCRIPTION_COLUMN, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(AMOUNT_COLUMN, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    editButton = new QPushButton("Edit selected");
    editButton->setObjectName("editButton");
    removeButton = new QPushButton("Remove selected");
    removeButton->setObjectName("removeButton");
    undoButton = new QPushButton("Undo");
    undoButton->setObjectName("undoButton");

    QHBoxLayout* listButtons = new QHBoxLayout;
    listButtons->addWidget(editButton);
    listButtons->addWidget(removeButton);
    listButtons->addStretch();
    listButtons->addWidget(undoButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(formButtons);
    layout->addWidget(messageLabel);
    layout->addWidget(table, 1);
    layout->addLayout(listButtons);

    connect(submitButton, &QPushButton::clicked, this, &ExpensesView::submitForm);
    connect(amountInput, &QLineEdit::returnPressed, this, &ExpensesView::submitForm);
    connect(descriptionInput, &QLineEdit::returnPressed, this, &ExpensesView::submitForm);
    connect(cancelButton, &QPushButton::clicked, this, &ExpensesView::cancelEditing);
    connect(editButton, &QPushButton::clicked, this, &ExpensesView::startEditingSelected);
    connect(table, &QTableWidget::cellDoubleClicked, this, &ExpensesView::startEditingSelected);
    connect(removeButton, &QPushButton::clicked, this, &ExpensesView::removeSelected);
    connect(undoButton, &QPushButton::clicked, this, &ExpensesView::undo);
    connect(table, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool hasSelection = selectedExpenseId() != NOT_EDITING;
        editButton->setEnabled(hasSelection);
        removeButton->setEnabled(hasSelection);
    });

    clearForm();
    refresh();
}

void ExpensesView::refresh() {
    const bool hasTerm = controller.hasTerm();
    for (QWidget* input : {static_cast<QWidget*>(amountInput), static_cast<QWidget*>(dateInput),
                           static_cast<QWidget*>(descriptionInput), static_cast<QWidget*>(submitButton)}) {
        input->setEnabled(hasTerm);
    }
    if (!hasTerm) {
        showMessage(false, ExpenseController::NO_TERM_MESSAGE);
    } else if (messageLabel->text() == ExpenseController::NO_TERM_MESSAGE) {
        messageLabel->clear();  // the term was just set up, so the hint is no longer true
    }
    if (editingId == NOT_EDITING) {
        // Keep a date the user picked if it is inside the term; otherwise move it in.
        dateInput->setDate(controller.suggestedDate(dateInput->date()));
    }

    const std::vector<Expense> expenses = controller.expensesNewestFirst();
    table->setRowCount(static_cast<int>(expenses.size()));
    const QLocale english(QLocale::English);
    for (int row = 0; row < static_cast<int>(expenses.size()); ++row) {
        const Expense& expense = expenses[row];
        QTableWidgetItem* dateItem = new QTableWidgetItem(english.toString(expense.date, ViewStyle::DATE_FORMAT));
        dateItem->setData(Qt::UserRole, expense.id);
        QTableWidgetItem* amountItem = new QTableWidgetItem(Money::format(expense.amount));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(row, DATE_COLUMN, dateItem);
        table->setItem(row, DESCRIPTION_COLUMN, new QTableWidgetItem(expense.description));
        table->setItem(row, AMOUNT_COLUMN, amountItem);
    }
    table->clearSelection();
    editButton->setEnabled(false);
    removeButton->setEnabled(false);
    undoButton->setEnabled(controller.canUndo());
    undoButton->setText(controller.undoLabel());
}

int ExpensesView::selectedExpenseId() const {
    const QList<QTableWidgetItem*> selected = table->selectedItems();
    if (selected.isEmpty()) {
        return NOT_EDITING;
    }
    return table->item(selected.first()->row(), DATE_COLUMN)->data(Qt::UserRole).toInt();
}

void ExpensesView::submitForm() {
    const ActionResult result =
        editingId == NOT_EDITING
            ? controller.addExpense(amountInput->text(), dateInput->date(), descriptionInput->text())
            : controller.updateExpense(editingId, amountInput->text(), dateInput->date(), descriptionInput->text());
    showMessage(result.ok, result.message);
    if (result.ok) {
        clearForm();
        emit changed();
    }
}

void ExpensesView::startEditingSelected() {
    const int expenseId = selectedExpenseId();
    const std::optional<Expense> expense = controller.findExpense(expenseId);
    if (!expense) {
        return;
    }
    editingId = expenseId;
    amountInput->setText(QString::number(expense->amount, 'f', 2));
    dateInput->setDate(expense->date);
    descriptionInput->setText(expense->description);
    submitButton->setText("Save changes");
    cancelButton->setVisible(true);
    showMessage(true, "Editing the expense. Change it and press Save changes.");
    amountInput->setFocus();
}

void ExpensesView::cancelEditing() {
    clearForm();
    messageLabel->clear();
}

void ExpensesView::removeSelected() {
    const int expenseId = selectedExpenseId();
    if (expenseId == NOT_EDITING) {
        return;
    }
    // No "are you sure" dialog: removing is safe because Undo brings it back.
    const ActionResult result = controller.removeExpense(expenseId);
    showMessage(result.ok, result.message);
    if (result.ok) {
        if (editingId == expenseId) {
            clearForm();
        }
        emit changed();
    }
}

void ExpensesView::undo() {
    const ActionResult result = controller.undoLastChange();
    showMessage(result.ok, result.message);
    if (result.ok) {
        // Undo can take away the expense being edited, so leave edit mode.
        if (editingId != NOT_EDITING) {
            clearForm();
        }
        emit changed();
    }
    // A refused undo is also used up, so the button state may have changed.
    undoButton->setEnabled(controller.canUndo());
    undoButton->setText(controller.undoLabel());
}

void ExpensesView::clearForm() {
    editingId = NOT_EDITING;
    amountInput->clear();
    descriptionInput->clear();
    dateInput->setDate(controller.suggestedDate(QDate::currentDate()));
    submitButton->setText("Add expense");
    cancelButton->setVisible(false);
}

void ExpensesView::showMessage(bool ok, const QString& text) {
    messageLabel->setStyleSheet(ViewStyle::messageStyle(ok));
    messageLabel->setText(text);
}
