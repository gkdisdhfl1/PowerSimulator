#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <expected>
#include <QVariantMap>

class SettingsUiController;
struct PresetPreviewData; // Controller와 데이터를 주고받을 구조체

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    // 의존성 주입을 위한 Setter
    void setController(SettingsUiController* controller);

signals:
    // Controller에게 작업을 요청하는 시그널들
    void saveAsPresetRequested(const QString& presetName);
    void loadPresetRequested(const QString& presetName);
    void deletePresetRequested(const QString& presetName);
    void renamePresetRequested(const QString& oldName, const QString& newName);
    void applyDialogSettingsRequested(int maxDataSize, double graphWidth);

public slots:
    // Controller로부터 작업 결과를 받는 슬롯
    void onControllerTaskFinished(const std::expected<void, std::string>& result, const QString& successMessage);
    // Controller로부터 프리셋 목록을 받는 슬롯
    void onPresetListChanged(const std::vector<std::string>& presetList);
    void onPresetValuesFetched(const QVariantMap& data);
    void onCurrentSettingsFetched(int maxDataSize, double graphWidth);

protected:
    // 다이얼 로그가 표시될 때마다 UI를 초기화
    void showEvent(QShowEvent *event) override;

private slots:
    void onNewPresetClicked();
    void onLoadPresetClicked();
    void onRenamePresetClicked();
    void onDeletePresetClicked();
    void onPresetSelectionChanged();
    void onApplyCurrentSettingsClicked();

private:
    Ui::SettingsDialog *ui;
    SettingsUiController* m_controller = nullptr; // Controller 포인터 소유x

    void refreshPresetList(); // Controller에게 프리셋 목록을 요청하는 함수
    void updateUiStates(); // 오른쪽 상세 설정 UI를 업데이트하는 함수
};

#endif // SETTINGS_DIALOG_H
