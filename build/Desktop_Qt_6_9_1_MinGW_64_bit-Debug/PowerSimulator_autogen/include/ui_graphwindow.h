/********************************************************************************
** Form generated from reading UI file 'graphwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GRAPHWINDOW_H
#define UI_GRAPHWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGraphicsView>

QT_BEGIN_NAMESPACE

class Ui_graphwindow
{
public:
    QGraphicsView *chartView;

    void setupUi(QDialog *graphwindow)
    {
        if (graphwindow->objectName().isEmpty())
            graphwindow->setObjectName("graphwindow");
        graphwindow->resize(400, 300);
        chartView = new QGraphicsView(graphwindow);
        chartView->setObjectName("chartView");
        chartView->setGeometry(QRect(40, 10, 331, 231));

        retranslateUi(graphwindow);

        QMetaObject::connectSlotsByName(graphwindow);
    } // setupUi

    void retranslateUi(QDialog *graphwindow)
    {
        graphwindow->setWindowTitle(QCoreApplication::translate("graphwindow", "GraphWindow", nullptr));
    } // retranslateUi

};

namespace Ui {
    class graphwindow: public Ui_graphwindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GRAPHWINDOW_H
