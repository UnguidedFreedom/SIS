#include "qmessageedit.h"

QMessageEdit::QMessageEdit(QWidget *parent) :
    QTextEdit(parent)
{
    setAcceptRichText(false);
}

void QMessageEdit::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        if(!(e == QKeySequence::InsertLineSeparator))
        {
            emit returnPressed();
            e->accept();
            return;
        }
    }
    else if(e == QKeySequence::NextChild)
    {
        emit nextTab();
        return;
    }
    else if(e == QKeySequence::PreviousChild)
    {
        emit previousTab();
        return;
    }
    QTextEdit::keyPressEvent(e);
}

void QMessageEdit::acceptKey(QKeyEvent *e)
{
    keyPressEvent(e);
}
