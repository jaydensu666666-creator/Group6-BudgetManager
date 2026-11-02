#ifndef ACTION_RESULT_H
#define ACTION_RESULT_H

#include <QString>

// What a controller hands back to a view after the user does something:
// whether it worked and the sentence to show them.
struct ActionResult {
    bool ok = false;
    QString message;

    static ActionResult success(const QString& message) { return {true, message}; }
    static ActionResult failure(const QString& message) { return {false, message}; }
};

#endif
