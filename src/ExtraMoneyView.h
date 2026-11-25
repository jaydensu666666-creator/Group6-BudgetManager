#ifndef EXTRA_MONEY_VIEW_H
#define EXTRA_MONEY_VIEW_H

#include <QWidget>

// Placeholder for the "Extra money" page (Zhaorong's feature: money that comes
// in outside the regular transfer, and the savings goal). The foundation adds
// the page so the tab order is final; the feature fills it in.
class ExtraMoneyView : public QWidget {
    Q_OBJECT
public:
    explicit ExtraMoneyView(QWidget* parent = nullptr);
};

#endif
