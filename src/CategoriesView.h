#ifndef CATEGORIES_VIEW_H
#define CATEGORIES_VIEW_H

#include <QWidget>

// Placeholder for the "Where it goes" page (Jayden's feature: categories, the
// spending per category, and the limits). The foundation adds the page so the
// tab order is final; the feature fills it in.
class CategoriesView : public QWidget {
    Q_OBJECT
public:
    explicit CategoriesView(QWidget* parent = nullptr);
};

#endif
