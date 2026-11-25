#include "CategoriesView.h"

#include <QLabel>
#include <QVBoxLayout>

#include "ViewStyle.h"

CategoriesView::CategoriesView(QWidget* parent) : QWidget(parent) {
    QLabel* title = new QLabel("Where it goes");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    QLabel* text = new QLabel(
        "Coming next: give each purchase a category and see this term's spending per category, "
        "with a limit for each category.");
    text->setWordWrap(true);
    text->setStyleSheet("color: " + ViewStyle::MUTED_COLOR + ";");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(text);
    layout->addStretch();
}
