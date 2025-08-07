#include "main_window.h"
#include "./ui_main_window.h"
#include "settings_dialog.h"
#include "simulation_engine.h"
#include "settings_manager.h"
#include <QInputDialog>
#include <QMessageBox>
#include "config.h"
#include <variant>

MainWindow::MainWindow(SimulationEngine *engine, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settingsDialog(new SettingsDialog(this))
    , m_engine(engine)
{
    ui->setupUi(this);

    // 데이터 베이스 설정
    QString dbPath = QApplication::applicationDirPath() + "/settings.db";
    m_settingsManager = std::make_unique<SettingsManager>(dbPath.toStdString());

    // UI 초기값 설정
    setupUiWidgets();
    createSignalSlotConnections();
    initializeSettingsMap();

    ui->splitter->setStretchFactor(0, 2);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleSettingButtonClicked()
{
    // qDebug() << "getMaxDataSize() = " << m_engine->getMaxDataSize();
    // qDebug() << "getGraphWidth() = " << ui->graphViewPlaceholder->getGraphWidth();

    // 다이얼로그를 열기 전에 현재 설정값으로 초기화
    m_settingsDialog->setInitialValues(
        m_engine->getMaxDataSize(),
        ui->graphViewPlaceholder->getGraphWidth()
    );

    if(m_settingsDialog->exec() == QDialog::Accepted) {
        // ok를 눌렀다면, 다이얼로그에서 새로운 값들을 가져와 적용
        m_engine->applySettings(
            m_settingsDialog->getMaxSize());
        ui->graphViewPlaceholder->setGraphWidth(m_settingsDialog->getGraphWidth());
    }
    // m_settingsDialog->open();
}

void MainWindow::onEngineRuninngStateChanged(bool isRunning)
{
    ui->startStopButton->setText(isRunning ? "일시정지" : "시작");
}

void MainWindow::onActionSaveSettings()
{
    bool ok;
    // 사용자에게 프리셋 이름을 입력받는 다이얼로그를 띄움
    QString presetName = QInputDialog::getText(this, "설정 저장", "저장할 설정의 이름을 입력하세요:", QLineEdit::Normal, "", &ok);

    if(ok && !presetName.isEmpty()) {
        saveUiToSettings(presetName.toStdString());
    }
}

void MainWindow::onActionLoadSettings()
{
    std::vector<std::string> presetStdVector = m_settingsManager->getAllPresetNames();
    if(presetStdVector.empty()) {
        QMessageBox::information(this, "알림", "저장된 설정이 없습니다.");
        return;
    }

    // std:vector<std::string>을 QStringList로 변환
    QStringList presetItems;
    for(const auto& name : presetStdVector) {
        presetItems << QString::fromStdString(name);
    }

    bool ok;
    // 사용자에게 프리셋 목록을 보여주고 선택받는 다이얼로그를 띄움
    QString selectedPreset = QInputDialog::getItem(this, "설정 불러오기", "불러올 설정을 선택하세요:", presetItems, 0, false, &ok);

    if(ok && !selectedPreset.isEmpty()) {
        applySettingsToUi(selectedPreset.toStdString());
    }
}

void MainWindow::onActionDeleteSettings()
{
    std::vector<std::string> presetStdVector = m_settingsManager->getAllPresetNames();
    if(presetStdVector.empty()) {
        QMessageBox::information(this, "알림", "삭제할 설정이 없습니다.");
        return;
    }

    // std:vector<std::string>을 QStringList로 변환
    QStringList presetItems;
    for(const auto& name : presetStdVector) {
        presetItems << QString::fromStdString(name);
    }

    bool ok;
    // 사용자에게 프리셋 목록을 보여주고 선택받는 다이얼로그를 띄움
    QString selectedPreset = QInputDialog::getItem(this, "설정 삭제", "삭제할 설정을 선택하세요:", presetItems, 0, false, &ok);

    if(ok && !selectedPreset.isEmpty()) {
        // 확인 메세지를 한번 더 보여줌
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "삭제 확인", QString("정말로 '%1' 설정을 삭제하겠습니까?").arg(selectedPreset), QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes) {
            m_settingsManager->deletePreset(selectedPreset.toStdString());
            ui->statusbar->showMessage(QString("'%1' 설정을 삭제했습니다.").arg(selectedPreset), 3000);
        }
    }
}

void MainWindow::setupUiWidgets()
{
    ui->voltageControlWidget->setRange(config::Source::Amplitude::Min, config::Source::Amplitude::Max);
    ui->voltageControlWidget->setValue(config::Source::Amplitude::Default);
    ui->voltageControlWidget->setSuffix(" V");

    ui->currentAmplitudeControl->setRange(config::Source::Current::MinAmplitude, config::Source::Current::MaxAmplitude);
    ui->currentAmplitudeControl->setValue(config::Source::Current::DefaultAmplitude);
    ui->currentAmplitudeControl->setSuffix(" A");

    ui->currentPhaseDial->setValue(config::Source::Current::DefaultPhaseOffset);

    ui->timeScaleWidget->setRange(config::TimeScale::Min, config::TimeScale::Max);
    ui->timeScaleWidget->setValue(config::TimeScale::Default);
    ui->timeScaleWidget->setSuffix(" x");

    ui->samplingCyclesControl->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    ui->samplingCyclesControl->setValue(config::Sampling::DefaultSamplingCycles);
    ui->samplesPerCycleControl->setRange(config::Sampling::MinValue, config::Sampling::maxValue);
    ui->samplesPerCycleControl->setValue(config::Sampling::DefaultSamplesPerCycle);

    ui->frequencyControlWidget->setRange(config::Source::Frequency::Min, config::Source::Frequency::Max);
    ui->frequencyControlWidget->setValue(config::Source::Frequency::Default);
    ui->frequencyControlWidget->setSuffix(" Hz");

    ui->currentPhaseDial->setValue(config::Source::Current::DefaultPhaseOffset);
    ui->currentPhaseLabel->setText(QString::number(ui->currentPhaseDial->value()) + " °");

    ui->perSampleRadioButton->setChecked(true);
}

void MainWindow::createSignalSlotConnections()
{
    connect(ui->settingButton, &QPushButton::clicked, this, &MainWindow::handleSettingButtonClicked);

    // 메뉴바 액션 연결
    connect(ui->actionSaveSettings, &QAction::triggered, this, &MainWindow::onActionSaveSettings);
    connect(ui->actionLoadSettings, &QAction::triggered, this, &MainWindow::onActionLoadSettings);
    connect(ui->actionDeleteSettings, &QAction::triggered, this, &MainWindow::onActionDeleteSettings);

    // ---- UI 이벤트 -> SimulationEngine 슬롯 ----
    connect(ui->startStopButton, &QPushButton::clicked, this, [this]() {
        if (m_engine->isRunning()) {
            m_engine->stop();
        } else {
            m_engine->start();
        }
    });

    // voltageControlWidget의 값이 바뀌면, 엔진의 현재 전압을 설정
    connect(ui->voltageControlWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setAmplitude);

    // timeScaleWidget 값이 바뀌면 엔진의 setTimeScale 슬롯 호출
    connect(ui->timeScaleWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setTimeScale);

    connect(ui->samplingCyclesControl, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setSamplingCycles);
    connect(ui->samplesPerCycleControl, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setSamplesPerCycle);

    connect(ui->frequencyControlWidget, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setFrequency);

    connect(ui->graphViewPlaceholder, &GraphWindow::redrawNeeded, m_engine, &SimulationEngine::onRedrawRequest);

    connect(ui->currentAmplitudeControl, &ValueControlWidget::valueChanged, m_engine, &SimulationEngine::setCurrentAmplitude);
    connect(ui->currentPhaseDial, &FineTuningDial::valueChanged, this, [this](int value) {
        // 엔진에 값 전달
        m_engine->setCurrentPhaseOffset(value);
        // 라벨 업데이트
        ui->currentPhaseLabel->setText(QString::number(value) + " °");
    });

    // 화면 갱신 주기 라디오 버튼
    connect(ui->perSampleRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if(checked)
            m_engine->setUpdateMode(SimulationEngine::UpdateMode::PerSample);
    });
    connect(ui->perHalfCycleRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if(checked)
            m_engine->setUpdateMode(SimulationEngine::UpdateMode::PerHalfCycle);
    });
    connect(ui->PerCycleRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if(checked)
            m_engine->setUpdateMode(SimulationEngine::UpdateMode::PerCycle);
    });
    // ----------------------


    // SimulationEngine 시그널 -> UI 슬롯
    connect(m_engine, &SimulationEngine::dataUpdated, ui->graphViewPlaceholder, &GraphWindow::updateGraph);
    connect(m_engine, &SimulationEngine::runningStateChanged, this, &MainWindow::onEngineRuninngStateChanged);

    // 그래프 관련
    connect(ui->autoScrollCheckBox, &QCheckBox::toggled, ui->graphViewPlaceholder, &GraphWindow::toggleAutoScroll);
    connect(ui->graphViewPlaceholder, &GraphWindow::autoScrollToggled, ui->autoScrollCheckBox, &QCheckBox::setChecked);
    connect(ui->graphViewPlaceholder, &GraphWindow::pointHovered, this, [this](const QPointF& point) {
        std::string coordText = std::format("시간: {:.3f} s, 전압: {:.3f} V", point.x(), point.y());
        if(ui->statusbar)
            ui->statusbar->showMessage(QString::fromStdString(coordText));
    });
}

void MainWindow::initializeSettingsMap()
{
    // 각 설정 항목의 정보를 맵에 등록
    m_settingsMap["voltageAmplitude"]   = {ui->voltageControlWidget, 220.0};
    m_settingsMap["currentAmplitude"]   = {ui->currentAmplitudeControl, 10.0};
    m_settingsMap["frequency"]   = {ui->frequencyControlWidget, 60.0};
    m_settingsMap["currentPhaseOffset"]   = {ui->currentPhaseDial, 0};
    m_settingsMap["timeScale"]   = {ui->timeScaleWidget, 1.0};
    m_settingsMap["samplingCycles"]   = {ui->samplingCyclesControl, 1.0};
    m_settingsMap["samplesPerCycle"]   = {ui->samplesPerCycleControl, 10};
}

void MainWindow::applySettingsToUi(std::string_view presetName)
{
    // 맵을 순회하면 DB에서 값을 불러와 각 위젯에 적용
    for(auto const& [key, info] : m_settingsMap) {
        std::visit([&](auto&& defaultValue) {
            // defaultValue의 실제 타입을 추론
            using T = std::decay_t<decltype(defaultValue)>;

            // DB에서 실제 타입에 맞는 기본값을 사용하여 설정을 불러옴
            T value=  m_settingsManager->loadSetting(presetName, key, defaultValue);

            // 위젯의 종류에 따라 불러온 값을 설정
            if(auto* control = qobject_cast<ValueControlWidget*>(info.widget)) {
                control->setValue(static_cast<double>(value));
            } else if(auto* dial = qobject_cast<QDial*>(info.widget)) {
                dial->setValue(static_cast<int>(value));
            }
        }, info.defaultValue);
    }

    // 라디오 버튼은 별도 처리
    int updateMode = m_settingsManager->loadSetting(presetName, "updateMode", 0);
    if(updateMode == 1) ui->perHalfCycleRadioButton->setChecked(true);
    else if(updateMode == 2) ui->PerCycleRadioButton->setChecked(true);
    else ui->perSampleRadioButton->setChecked(true);

    ui->statusbar->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
}

void MainWindow::saveUiToSettings(std::string_view presetName)
{
    // 맵을 순회하며 각 위젯의 현재 값을 DB에 저장
    for(auto const& [key, info] : m_settingsMap) {
        if(auto* control = qobject_cast<ValueControlWidget*>(info.widget)) {
            m_settingsManager->saveSetting(presetName, key, control->value());
        } else if(auto* dial = qobject_cast<QDial*>(info.widget)) {
            m_settingsManager->saveSetting(presetName, key, dial->value());
        }
    }

    // 라디오 버튼은 별도 처리
    int updateMode = ui->perSampleRadioButton->isChecked() ? 0 : (ui->perHalfCycleRadioButton->isChecked() ? 1 : 2);
    m_settingsManager->saveSetting(presetName, "updateMode", updateMode);

    ui->statusbar->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
}
