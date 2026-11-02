#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H

#include <QWidget>

class QLabel;
class QPushButton;
class StatusController;

// Home page: "Where I stand". Money left to spend is the biggest thing on the
// screen because it is what the user opens the app to check (P2 sketch).
class StatusView : public QWidget {
    Q_OBJECT
public:
    explicit StatusView(StatusController& controller, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void setUpTermRequested();

private:
    StatusController& controller;
    QLabel* moneyLeftLabel;
    QLabel* spendableLabel;
    QLabel* weekLabel;
    QLabel* paceLabel;
    QLabel* detailsLabel;
    QPushButton* setUpButton;
};

#endif
