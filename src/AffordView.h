#ifndef AFFORD_VIEW_H
#define AFFORD_VIEW_H

#include <QWidget>

// Placeholder for the "Can I afford it?" page (Tianbin's feature: type a price
// and see what it would do to the money left and this week's amount). The
// foundation adds the page so the tab order is final; the feature fills it in.
class AffordView : public QWidget {
    Q_OBJECT
public:
    explicit AffordView(QWidget* parent = nullptr);
};

#endif
