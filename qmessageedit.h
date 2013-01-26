#ifndef QMESSAGEEDIT_H
#define QMESSAGEEDIT_H

#include <QTextEdit>
#include <QKeyEvent>
#include "qtabswidget.h"

class QMessageEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit QMessageEdit(QWidget *parent = 0);

protected:
    virtual void keyPressEvent(QKeyEvent *e);

signals:
    void returnPressed();
    void nextTab();
    void previousTab();
};

#endif // QMESSAGEEDIT_H
