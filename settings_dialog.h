#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include "simulation_engine.h"

#include <QDialog>
#include <expected>
#include <QVariantMap>

class SettingsUiController;
class QListWidget;
class QPushButton;
class QTableWidget;
class QSpinBox;
class QDoubleSpinBox;
class QGroupBox;

struct PresetPreviewData; // Controller와 데이터를 주고받을 구조체

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(SettingsUiController* controller, QWidget *parent = nullptr);
    ~SettingsDialog();

    enum class DialogResult { Accepted, PresetLoaded, Cancled };

    int openWithValues(const SimulationEngine::Parameters& params); // 다이얼 열고 초기화하는 함수
    DialogResult getResultState() const;

    int getMaxSize() const;
    double getGraphWidth() const;

signals:
    // Controller에게 작업을 요청하는 시그널들
    void saveAsPresetRequested(const QString& presetName);
    void loadPresetRequested(const QString& presetName);
    void deletePresetRequested(const QString& presetName);
    void renamePresetRequested(const QString& oldName, const QString& newName);
    void settingsApplied(const SimulationEngine::Parameters& params);

public slots:
    // Controller로부터 작업 결과를 받는 슬롯
    void onControllerTaskFinished(const std::expected<void, std::string>& result, const QString& successMessage);
    // Controller로부터 프리셋 목록을 받는 슬롯
    void onPresetListChanged(const std::vector<std::string>& presetList);
    void onPresetValuesFetched(const QVariantMap& data);
    void onCurrentSettingsFetched(SimulationEngine::Parameters& params);

private slots:
    void onNewPresetClicked();
    void onLoadPresetClicked();
    void onRenamePresetClicked();
    void onDeletePresetClicked();
    void onPresetSelectionChanged();

private:
    // --- UI 요소들의 멤버 변수 ---
    QListWidget* m_presetListWidget;
    QPushButton* m_newPresetButton;
    QPushButton* m_loadPresetButton;
    QPushButton* m_deletePresetButton;
    QPushButton* m_renamePresetButton;
    QTableWidget* m_previewTableWidget;
    QSpinBox* m_maxDataSizeSpinBox;
    QDoubleSpinBox* m_graphWidthSpinBox;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QGroupBox* m_previewGroupBox;

    // 3상 설정 위젯 포인터
    QDoubleSpinBox *m_voltage_B_AmplitudeSpinBox;
    QDoubleSpinBox *m_voltage_B_PhaseSpinBox;
    QDoubleSpinBox *m_voltage_C_AmplitudeSpinBox;
    QDoubleSpinBox *m_voltage_C_PhaseSpinBox;
    QDoubleSpinBox *m_current_B_AmplitudeSpinBox;
    QDoubleSpinBox *m_current_B_PhaseSpinBox;
    QDoubleSpinBox *m_current_C_AmplitudeSpinBox;
    QDoubleSpinBox *m_current_C_PhaseSpinBox;

    SettingsUiController* m_controller = nullptr; // Controller 포인터 소유x
    DialogResult m_resultState;

    void setupUi();
    void refreshPresetList(); // Controller에게 프리셋 목록을 요청하는 함수
    void updateUiStates(); // 오른쪽 상세 설정 UI를 업데이트하는 함수
    void accept() override;
};

#endif // SETTINGS_DIALOG_H
