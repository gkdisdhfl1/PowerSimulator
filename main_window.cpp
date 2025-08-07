#include "main_window.h"
#include "./ui_main_window.h"
#include "settings_dialog.h"
#include "simulation_engine.h"
#include "settings_manager.h"
#include <QInputDialog>
#include <QMessageBox>
#include <variant>
#include "config.h"

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
        auto result = saveUiToSettings(presetName.toStdString());
        if(!result)
            QMessageBox::warning(this, "저장 실패", QString::fromStdString(result.error()));
    }
}

void MainWindow::onActionLoadSettings()
{
    auto presetsResult = m_settingsManager->getAllPresetNames();
    if(!presetsResult) {
        QMessageBox::warning(this, "오류", QString::fromStdString(presetsResult.error()));
        return;
    }

    // std:vector<std::string>을 QStringList로 변환
    QStringList presetItems;
    for(const auto& name : presetsResult.value()) {
        presetItems << QString::fromStdString(name);
    }

    bool ok;
    // 사용자에게 프리셋 목록을 보여주고 선택받는 다이얼로그를 띄움
    QString selectedPreset = QInputDialog::getItem(this, "설정 불러오기", "불러올 설정을 선택하세요:", presetItems, 0, false, &ok);

    if(ok && !selectedPreset.isEmpty()) {
        auto result = applySettingsToUi(selectedPreset.toStdString());
        if(!result)
            QMessageBox::warning(this, "불러오기 실패", QString::fromStdString(result.error()));
    }
}

void MainWindow::onActionDeleteSettings()
{
    auto presetsResult = m_settingsManager->getAllPresetNames();
    if(!presetsResult) {
        QMessageBox::warning(this, "오류", QString::fromStdString(presetsResult.error()));
        return;
    }

    if(presetsResult.value().empty()) {
        QMessageBox::information(this, "알림", "삭제할 설정이 없습니다.");
        return;
    }

    // std:vector<std::string>을 QStringList로 변환
    QStringList presetItems;
    for(const auto& name : presetsResult.value()) {
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
            auto result = m_settingsManager->deletePreset(selectedPreset.toStdString());
            if(result) {
                ui->statusbar->showMessage(QString("'%1' 설정을 삭제했습니다.").arg(selectedPreset), 3000);
            } else {
                QMessageBox::warning(this, "삭제 실패: ", QString::fromStdString(result.error()));
            }
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
    m_settingsMap["voltageAmplitude"]   = {
        [this] {return ui->voltageControlWidget->value();},
        [this](const SettingValue& val) {ui->voltageControlWidget->setValue(std::get<double>(val));},
        config::Source::Amplitude::Default
    };
    m_settingsMap["currentAmplitude"]  = {
        [this] { return ui->currentAmplitudeControl->value();},
        [this](const SettingValue& val) {ui->currentAmplitudeControl->setValue(std::get<double>(val));},
        config::Source::Current::DefaultAmplitude
    };
    m_settingsMap["frequency"] = {
        [this] { return ui->frequencyControlWidget->value();},
        [this](const SettingValue& val) {ui->frequencyControlWidget->setValue(std::get<double>(val));},
        config::Source::Frequency::Default
    };
    m_settingsMap["currentPhaseOffset"] = {
        [this] { return ui->currentPhaseDial->value();},
        [this](const SettingValue& val){ui->currentPhaseDial->setValue(std::get<int>(val));},
        config::Source::Current::DefaultPhaseOffset
    };
    m_settingsMap["timeScale"] = {
        [this] { return ui->timeScaleWidget->value(); },
        [this](const SettingValue& val) { ui->timeScaleWidget->setValue(std::get<double>(val));},
        config::TimeScale::Default
    };
    m_settingsMap["samplingCycles"] = {
        [this] { return ui->samplingCyclesControl->value(); },
        [this](const SettingValue& val) { ui->samplingCyclesControl->setValue(std::get<double>(val));},
        config::Sampling::DefaultSamplingCycles
    };
    m_settingsMap["samplesPerCycle"] = {
        [this] { return static_cast<int>(ui->samplesPerCycleControl->value());},
        [this](const SettingValue& val) { ui->samplesPerCycleControl->setValue(std::get<int>(val));},
        config::Sampling::DefaultSamplesPerCycle
    };
    m_settingsMap["maxDataSize"] = {
        [this] { return m_engine->getMaxDataSize(); },
        [this](const SettingValue& val) { m_engine->applySettings(std::get<int>(val));},
        config::Simulation::DefaultDataSize
    };
    m_settingsMap["graphWidthSec"] = {
        [this] { return ui->graphViewPlaceholder->getGraphWidth();},
        [this](const SettingValue& val) { ui->graphViewPlaceholder->setGraphWidth(std::get<double>(val));},
        config::View::GraphWidth::Default
    };

    m_settingsMap["updateMode"] = {
        [this]() -> SettingValue {
            if(ui->perHalfCycleRadioButton->isChecked()) return 1;
            if(ui->PerCycleRadioButton->isChecked()) return 2;
            return 0;
        },
        [this](const SettingValue& val) {
            int mode = std::get<int>(val);
            if(mode == 1) ui->perHalfCycleRadioButton->setChecked(true);
            else if(mode == 2) ui->PerCycleRadioButton->setChecked(true);
            else ui->perSampleRadioButton->setChecked(true);
        },
        0
    };
}

std::expected<void, std::string> MainWindow::applySettingsToUi(std::string_view presetName)
{
    // 맵을 순회하면 DB에서 값을 불러와 각 setter에 적용
    for(auto const& [key, info] : m_settingsMap) {
        auto result = std::visit([&](auto&& defaultValue)->std::expected<void, std::string> {
            // defaultValue의 실제 타입을 추론
            using T = std::decay_t<decltype(defaultValue)>;

            // DB에서 실제 타입에 맞는 기본값을 사용하여 설정을 불러옴
            auto loadResult=  m_settingsManager->loadSetting(presetName, key, defaultValue);
            if(!loadResult) {
                return std::unexpected(loadResult.error());
            }

            qDebug() << "Applying:" << QString::fromStdString(key)
                     << "| Loaded value: " << QVariant::fromValue(*loadResult)
                     << "| Type:" << typeid(*loadResult).name();

            info.setter(*loadResult); // setter에 T 타입의 값을 담은 variant 전달
            return {};
        }, info.defaultValue);

        if(!result) return result; // 오류 발생 시 즉시 전파
    }

    ui->statusbar->showMessage(QString("'%1' 설정을 불러왔습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
    return {};
}

std::expected<void, std::string> MainWindow::saveUiToSettings(std::string_view presetName)
{
    // 맵을 순회하며 각 getter를 호출하여 값을 DB에 저장
    for(auto const& [key, info] : m_settingsMap) {
        auto result = std::visit([&](auto&& value) {
            return m_settingsManager->saveSetting(presetName, key, value);
        }, info.getter());

        if(!result) return result; // 오류 발생 시 즉시 전파
    }

    ui->statusbar->showMessage(QString("'%1' 이름으로 설정을 저장했습니다.").arg(QString::fromUtf8(presetName.data(), presetName.size())), 3000);
    return {};
}
