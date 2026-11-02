#ifndef TERM_SETUP_VIEW_H
#define TERM_SETUP_VIEW_H

#include <QWidget>

class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class TermController;

// Term setup page: how much money came in, how much to set aside, and when
// the next transfer arrives.
class TermSetupView : public QWidget {
    Q_OBJECT
public:
    explicit TermSetupView(TermController& controller, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void changed();

private slots:
    void save();

private:
    TermController& controller;
    QLineEdit* receivedInput;
    QLineEdit* savingsInput;
    QDateEdit* startInput;
    QDateEdit* nextTransferInput;
    QPushButton* saveButton;
    QLabel* messageLabel;
};

#endif
