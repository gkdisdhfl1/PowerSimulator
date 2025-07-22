#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // MainWindow에서 초기값을 설정해주기 위한 함수
    void setInitialValues(int maxSize, double graphWidth);
    void accept() override;

signals:
    void settingsApplied(double interval, int maxSize, double graphWidth);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGS_DIALOG_H
