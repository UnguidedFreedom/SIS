#include "qwindow.h"

QWindow::QWindow(QWidget *parent) :
    QMainWindow(parent)
{
    tabs = 0;
    settings = NULL;

    window = new QTabsWidget;
    window->setTabsClosable(true);
    window->setMovable(true);
    connect(window, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    tabBar = window->tabBar();
    connect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(clearColor(int)));
    connect(tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(moveTab(int,int)));

    setCentralWidget(window);
}

int QWindow::currentIndex()
{
    return window->currentIndex();
}

void QWindow::setTabTextColor(int tab, QColor color)
{
    tabBar->setTabTextColor(tab, color);
}

int QWindow::addTab(QWidget* widget, QString string)
{
    tabs++;
    return window->addTab(widget, string);
}

void QWindow::clear()
{
    window->clear();
    tabs = 0;
}

int QWindow::count()
{
    return tabs;
}

void QWindow::closeTab(int tab)
{
    window->removeTab(tab);
    tabs--;
    emit tabCloseRequested(tab);
}

void QWindow::clearColor(int tab)
{
    tabBar->setTabTextColor(tab, Qt::black);
}

void QWindow::moveTab(int from, int to)
{
    emit tabMoved(from, to);
}

void QWindow::previousTab()
{
    window->previousTab();
}

void QWindow::nextTab()
{
    window->nextTab();
}

void QWindow::setSettings(QSettings *tmpSettings)
{
    settings = tmpSettings;
}

void QWindow::update()
{
    if(tabs == 0)
        close();
    else if(tabs == 1)
        tabBar->setVisible(false);
    else
        tabBar->setVisible(true);
}

void QWindow::closeEvent(QCloseEvent *event)
{
    settings->setValue("dimensions", this->geometry());
    settings->setValue("maximized", this->isMaximized());
    event->accept();
}
