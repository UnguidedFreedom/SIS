#include "qmessagesbrowser.h"

QMessagesBrowser::QMessagesBrowser(QWidget *parent) :
    QTextBrowser(parent)
{
}

void QMessagesBrowser::keyPressEvent(QKeyEvent *ev)
{
    if(ev->modifiers() == Qt::NoModifier || ev->modifiers() == Qt::ShiftModifier)
        emit giveFocus(ev->text());
    QTextBrowser::keyPressEvent(ev);
}
