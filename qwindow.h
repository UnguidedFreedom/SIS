#ifndef QWINDOW_H
#define QWINDOW_H

#include <QtGui>
#include "qtabswidget.h"

class QWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit QWindow(QWidget *parent = 0);
    int currentIndex();
    void setTabTextColor(int, QColor);
    int addTab(QWidget*, QString);
    void clear();
    int count();
    void setSettings(QSettings* tmpSettings);
    void update();
    
signals:
    void tabCloseRequested(int);
    void tabMoved(int, int);
    
public slots:
    void closeTab(int);
    void clearColor(int);
    void moveTab(int, int);
    void nextTab();
    void previousTab();

private:
    QTabsWidget* window;
    QTabBar* tabBar;
    int tabs;
    QSettings* settings;
    virtual void closeEvent(QCloseEvent *event);
};

#include "sis.h"
#endif // QWINDOW_H
