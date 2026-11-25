#include "AffordView.h"

#include <QLabel>
#include <QVBoxLayout>

#include "ViewStyle.h"

AffordView::AffordView(QWidget* parent) : QWidget(parent) {
    QLabel* title = new QLabel("Can I afford it?");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    QLabel* text = new QLabel(
        "Coming next: type a price before you buy and see the money left, this week's amount, "
        "and whether your money still lasts until the next transfer.");
    text->setWordWrap(true);
    text->setStyleSheet("color: " + ViewStyle::MUTED_COLOR + ";");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(text);
    layout->addStretch();
}
