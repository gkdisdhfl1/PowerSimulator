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
#include <QtWidgets/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_SettingsDialog
{
public:
    QPushButton *OkButton;
    QPushButton *CancelButton;
    QLabel *label;
    QDoubleSpinBox *intervalSpinBox;
    QLabel *label_2;
    QSpinBox *maxSizeSpinBox;

    void setupUi(QDialog *SettingsDialog)
    {
        if (SettingsDialog->objectName().isEmpty())
            SettingsDialog->setObjectName("SettingsDialog");
        SettingsDialog->resize(400, 300);
        OkButton = new QPushButton(SettingsDialog);
        OkButton->setObjectName("OkButton");
        OkButton->setGeometry(QRect(220, 260, 75, 24));
        OkButton->setCursor(QCursor(Qt::CursorShape::ArrowCursor));
        CancelButton = new QPushButton(SettingsDialog);
        CancelButton->setObjectName("CancelButton");
        CancelButton->setGeometry(QRect(310, 260, 75, 24));
        label = new QLabel(SettingsDialog);
        label->setObjectName("label");
        label->setGeometry(QRect(30, 80, 71, 31));
        QFont font;
        font.setPointSize(12);
        label->setFont(font);
        intervalSpinBox = new QDoubleSpinBox(SettingsDialog);
        intervalSpinBox->setObjectName("intervalSpinBox");
        intervalSpinBox->setGeometry(QRect(130, 80, 211, 31));
        label_2 = new QLabel(SettingsDialog);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(30, 130, 71, 31));
        label_2->setFont(font);
        maxSizeSpinBox = new QSpinBox(SettingsDialog);
        maxSizeSpinBox->setObjectName("maxSizeSpinBox");
        maxSizeSpinBox->setGeometry(QRect(130, 130, 211, 31));

        retranslateUi(SettingsDialog);
        QObject::connect(OkButton, &QPushButton::clicked, SettingsDialog, qOverload<>(&QDialog::accept));
        QObject::connect(CancelButton, &QPushButton::clicked, SettingsDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(SettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *SettingsDialog)
    {
        SettingsDialog->setWindowTitle(QCoreApplication::translate("SettingsDialog", "SettingDialog", nullptr));
        OkButton->setText(QCoreApplication::translate("SettingsDialog", "Ok", nullptr));
        CancelButton->setText(QCoreApplication::translate("SettingsDialog", "Cancel", nullptr));
        label->setText(QCoreApplication::translate("SettingsDialog", "\354\213\234\352\260\204 \352\260\204\352\262\251", nullptr));
        label_2->setText(QCoreApplication::translate("SettingsDialog", "\354\240\200\354\236\245 \355\201\254\352\270\260", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SettingsDialog: public Ui_SettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGSDIALOG_H
