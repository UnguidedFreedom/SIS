#include "qwindow.h"

QWindow::QWindow(QWidget *parent) :
    QMainWindow(parent)
{
    tabs = 0;
    settings = NULL;

    tabBar = new QTabBar;
    tabBar->setTabsClosable(true);
    tabBar->setMovable(true);
    tabBar->setExpanding(true);

    conversations = new QStackedWidget;

    connect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(clearColor(int)));
    connect(tabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(moveTab(int,int)));

    QWidget* container = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(tabBar);
    layout->addWidget(conversations);
    container->setLayout(layout);
    setCentralWidget(container);
}

int QWindow::currentIndex()
{
    return tabBar->currentIndex();
}

void QWindow::setTabTextColor(int tab, QColor color)
{
    tabBar->setTabTextColor(tab, color);
}

int QWindow::addTab(QWidget* widget, QString string)
{
    tabs++;
    int current = conversations->addWidget(widget);
    int value = tabBar->addTab(string);
    tabBar->setTabData(value, current);
    return value;
}

void QWindow::clear()
{
    int count = tabBar->count();
    for(int i=0; i<count; i++)
    {
        int index = tabBar->tabData(0).toInt();
        conversations->removeWidget(conversations->widget(index));
        tabBar->removeTab(0);
        const int count = tabBar->count();
        for(int i=0; i<count; i++)
        {
            int data = tabBar->tabData(i).toInt();
            if(data > index)
                tabBar->setTabData(i, data-1);
        }
    }

    tabs = 0;
}

int QWindow::count()
{
    return tabs;
}

void QWindow::closeTab(int tab)
{
    int index = tabBar->tabData(tab).toInt();
    conversations->removeWidget(conversations->widget(index));
    tabBar->removeTab(tab);
    const int count = tabBar->count();
    for(int i=0; i<count; i++)
    {
        int data = tabBar->tabData(i).toInt();
        if(data > index)
            tabBar->setTabData(i, data-1);
    }
    tabs--;
    emit tabCloseRequested(tab);
}

void QWindow::clearColor(int tab)
{
    tabBar->setTabTextColor(tab, Qt::black);
    conversations->setCurrentIndex(tabBar->tabData(tab).toInt());
}

void QWindow::moveTab(int from, int to)
{
    emit tabMoved(from, to);
}

void QWindow::previousTab()
{
    int count = tabBar->count();
    if(count > 1)
        tabBar->setCurrentIndex((tabBar->currentIndex()+1)%count);
}

void QWindow::nextTab()
{
    int count = tabBar->count();
    if(count > 1)
        tabBar->setCurrentIndex((tabBar->currentIndex()+count-1)%count);
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
    clear();
    settings->setValue("dimensions", this->geometry());
    settings->setValue("maximized", this->isMaximized());
    event->accept();
}
