/********************************************************************************
** Form generated from reading UI file 'SettingsDialog.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGSDIALOG_H
#define UI_SETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_SettingsDialog
{
public:
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QLabel *label;
    QDoubleSpinBox *doubleSpinBox;

    void setupUi(QDialog *SettingsDialog)
    {
        if (SettingsDialog->objectName().isEmpty())
            SettingsDialog->setObjectName("SettingsDialog");
        SettingsDialog->resize(400, 300);
        pushButton = new QPushButton(SettingsDialog);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(220, 260, 75, 24));
        pushButton_2 = new QPushButton(SettingsDialog);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(310, 260, 75, 24));
        label = new QLabel(SettingsDialog);
        label->setObjectName("label");
        label->setGeometry(QRect(30, 80, 71, 31));
        QFont font;
        font.setPointSize(12);
        label->setFont(font);
        doubleSpinBox = new QDoubleSpinBox(SettingsDialog);
        doubleSpinBox->setObjectName("doubleSpinBox");
        doubleSpinBox->setGeometry(QRect(130, 80, 211, 31));

        retranslateUi(SettingsDialog);

        QMetaObject::connectSlotsByName(SettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *SettingsDialog)
    {
        SettingsDialog->setWindowTitle(QCoreApplication::translate("SettingsDialog", "SettingDialog", nullptr));
        pushButton->setText(QCoreApplication::translate("SettingsDialog", "Okay", nullptr));
        pushButton_2->setText(QCoreApplication::translate("SettingsDialog", "Cancel", nullptr));
        label->setText(QCoreApplication::translate("SettingsDialog", "\354\213\234\352\260\204 \352\260\204\352\262\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SettingsDialog: public Ui_SettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGSDIALOG_H
