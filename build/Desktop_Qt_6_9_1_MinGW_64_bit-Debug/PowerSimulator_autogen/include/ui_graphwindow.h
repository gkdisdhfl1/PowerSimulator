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
#include <QtWidgets/QGridLayout>

QT_BEGIN_NAMESPACE

class Ui_GraphWindow
{
public:
    QGridLayout *gridLayout;
    QGraphicsView *chartView;

    void setupUi(QDialog *GraphWindow)
    {
        if (GraphWindow->objectName().isEmpty())
            GraphWindow->setObjectName("GraphWindow");
        GraphWindow->resize(400, 300);
        gridLayout = new QGridLayout(GraphWindow);
        gridLayout->setObjectName("gridLayout");
        chartView = new QGraphicsView(GraphWindow);
        chartView->setObjectName("chartView");

        gridLayout->addWidget(chartView, 0, 0, 1, 1);


        retranslateUi(GraphWindow);

        QMetaObject::connectSlotsByName(GraphWindow);
    } // setupUi

    void retranslateUi(QDialog *GraphWindow)
    {
        GraphWindow->setWindowTitle(QCoreApplication::translate("GraphWindow", "GraphWindow", nullptr));
    } // retranslateUi

};

namespace Ui {
    class GraphWindow: public Ui_GraphWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GRAPHWINDOW_H
