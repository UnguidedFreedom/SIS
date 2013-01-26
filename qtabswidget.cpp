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
