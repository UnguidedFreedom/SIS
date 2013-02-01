/*
SIS - SIS Is Secure - safe instant messaging based on a non-central server architecture
Copyright (C) 2013 Benoît Vernier <vernier.benoit@gmail.com>

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

#include "qtabswidget.h"

QTabsWidget::QTabsWidget(QWidget *parent) :
    QTabWidget(parent)
{
}

void QTabsWidget::nextTab()
{
    if(count() > 1)
        setCurrentIndex((currentIndex()+1)%count());
}

void QTabsWidget::previousTab()
{
    if(count() > 1)
        setCurrentIndex((currentIndex()+count()-1)%count());
}
