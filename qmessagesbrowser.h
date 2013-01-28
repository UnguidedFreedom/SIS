#ifndef QMESSAGESBROWSER_H
#define QMESSAGESBROWSER_H

#include <QTextBrowser>
#include <QKeyEvent>

class QMessagesBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit QMessagesBrowser(QWidget *parent = 0);
    
signals:
    void giveFocus(QKeyEvent*);
protected:
    virtual void keyPressEvent(QKeyEvent *ev);
};

#endif // QMESSAGESBROWSER_H
