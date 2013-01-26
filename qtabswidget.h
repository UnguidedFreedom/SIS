#ifndef QTABSWIDGET_H
#define QTABSWIDGET_H

#include <QTabWidget>
#include <QKeyEvent>

class QTabsWidget : public QTabWidget
{
    Q_OBJECT
public:
    QTabsWidget(QWidget *parent = 0);
    ~QTabsWidget() {}

    QTabBar* tabBar() const { return QTabWidget::tabBar(); }
public slots:
    void nextTab();
    void previousTab();
};


#endif // QTABSWIDGET_H
