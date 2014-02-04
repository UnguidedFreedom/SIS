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

#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QtGui>

class Conversation : public QMainWindow
{
    Q_OBJECT
public:
    explicit Conversation(QWidget *parent = 0);
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
    void closed();
    
public slots:
    void closeTab(int);
    void clearColor(int);
    void moveTab(int, int);
    void nextTab();
    void previousTab();

private:
    QTabBar* tabBar;
    QStackedWidget* conversations;
    int tabs;
    QSettings* settings;
    virtual void closeEvent(QCloseEvent *event);
};

#include "sis.h"
#endif // CONVERSATION_H
