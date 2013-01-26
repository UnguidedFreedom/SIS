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
    if(e == QKeySequence::NextChild)
    {
        emit nextTab();
        return;
    }
    QTextEdit::keyPressEvent(e);
}
