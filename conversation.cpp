/*
SIS - SIS Is Secure - safe instant messaging based on a non-central server architecture
Copyright (C) 2013-2014 Benoît Vernier <vernier.benoit@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "conversation.h"

Conversation::Conversation(QWidget *parent) :
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

int Conversation::currentIndex()
{
    return tabBar->currentIndex();
}

void Conversation::setTabTextColor(int tab, QColor color)
{
    tabBar->setTabTextColor(tab, color);
}

int Conversation::addTab(QWidget* widget, QString string)
{
    tabs++;
    int current = conversations->addWidget(widget);
    int value = tabBar->addTab(string);
    tabBar->setTabData(value, current);
    return value;
}

void Conversation::clear()
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

int Conversation::count()
{
    return tabs;
}

void Conversation::closeTab(int tab)
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

void Conversation::clearColor(int tab)
{
    tabBar->setTabTextColor(tab, Qt::black);
    conversations->setCurrentIndex(tabBar->tabData(tab).toInt());
}

void Conversation::moveTab(int from, int to)
{
    emit tabMoved(from, to);
}

void Conversation::previousTab()
{
    int count = tabBar->count();
    if(count > 1)
        tabBar->setCurrentIndex((tabBar->currentIndex()+1)%count);
}

void Conversation::nextTab()
{
    int count = tabBar->count();
    if(count > 1)
        tabBar->setCurrentIndex((tabBar->currentIndex()+count-1)%count);
}

void Conversation::setSettings(QSettings *tmpSettings)
{
    settings = tmpSettings;
}

void Conversation::update()
{
    if(tabs == 0)
        close();
    else if(tabs == 1)
        tabBar->setVisible(false);
    else
        tabBar->setVisible(true);
}

void Conversation::closeEvent(QCloseEvent *event)
{
    clear();
    settings->setValue("dimensions", this->geometry());
    settings->setValue("maximized", this->isMaximized());
    event->accept();
}
