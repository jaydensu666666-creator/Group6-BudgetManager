#include "ExtraMoneyView.h"

#include <QLabel>
#include <QVBoxLayout>

#include "ViewStyle.h"

ExtraMoneyView::ExtraMoneyView(QWidget* parent) : QWidget(parent) {
    QLabel* title = new QLabel("Extra money");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    QLabel* text = new QLabel(
        "Coming next: record money that arrives outside the regular transfer, choose to spend it "
        "or save it, and set a savings goal with how much is saved and how much is still missing.");
    text->setWordWrap(true);
    text->setStyleSheet("color: " + ViewStyle::MUTED_COLOR + ";");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(text);
    layout->addStretch();
}
