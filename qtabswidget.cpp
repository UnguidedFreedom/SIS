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
