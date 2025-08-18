#include "settings_dialog.h"
#include "config.h"
#include "settings_ui_controller.h"
#include "ui_settings_dialog.h"
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

SettingsDialog::SettingsDialog(SettingsUiController* controller ,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_controller(controller)
    , m_resultState(DialogResult::Cancled)
{
    ui->setupUi(this);
    ui->maxDataSizeSpinBox->setRange(config::Simulation::MinDataSize, config::Simulation::MaxDataSize);

    ui->graphWidthSpinBox->setSuffix(" s");
    ui->graphWidthSpinBox->setRange(config::View::GraphWidth::Min, config::View::GraphWidth::Max);
    ui->graphWidthSpinBox->setValue(config::View::GraphWidth::Default);

    // dialog -> controller
    connect(this, &SettingsDialog::saveAsPresetRequested, m_controller, &SettingsUiController::onSaveAsPresetRequested);
    connect(this, &SettingsDialog::loadPresetRequested, m_controller, &SettingsUiController::onLoadPresetRequested);
    connect(this, &SettingsDialog::deletePresetRequested, m_controller, &SettingsUiController::onDeletePresetRequested);
    connect(this, &SettingsDialog::renamePresetRequested, m_controller, &SettingsUiController::onRenamePresetRequested);
    connect(this, &SettingsDialog::settingsApplied, m_controller, &SettingsUiController::onApplyDialogSettings);

    // controller -> dialog
    connect(m_controller, &SettingsUiController::taskFinished, this, &SettingsDialog::onControllerTaskFinished);
    connect(m_controller, &SettingsUiController::presetListChanged, this, &SettingsDialog::onPresetListChanged);
    connect(m_controller, &SettingsUiController::presetValuesFetched, this, &SettingsDialog::onPresetValuesFetched);

    // UI에 있는 버튼들의 시그널을 이 클래스의 private 슬롯에 연결
    connect(ui->newPresetButton, &QPushButton::clicked, this, &SettingsDialog::onNewPresetClicked);
    connect(ui->loadPresetButton, &QPushButton::clicked, this, &SettingsDialog::onLoadPresetClicked);
    connect(ui->renamePresetButton, &QPushButton::clicked, this, &SettingsDialog::onRenamePresetClicked);
    connect(ui->deletePresetButton, &QPushButton::clicked, this, &SettingsDialog::onDeletePresetClicked);

    // 리스트 위젯의 선택이 변경될 때의 시그널을 슬롯에 연결
    connect(ui->presetListWidget, &QListWidget::currentItemChanged, this, &SettingsDialog::onPresetSelectionChanged);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

int SettingsDialog::openWithValues(int currentMaxSize, double currentGraphWidth)
{
    ui->maxDataSizeSpinBox->setValue(currentMaxSize);
    ui->graphWidthSpinBox->setValue(currentGraphWidth);
    refreshPresetList();
    updateUiStates();

    m_resultState = DialogResult::Cancled;
    return exec();
}

int SettingsDialog::getMaxSize() const
{
    return ui->maxDataSizeSpinBox->value();
}
double SettingsDialog::getGraphWidth() const
{
    return ui->graphWidthSpinBox->value();
}
SettingsDialog::DialogResult SettingsDialog::getResultState() const
{
    return m_resultState;
}

// ----- 슬롯 구현 -----
void SettingsDialog::onNewPresetClicked()
{
    if(!m_controller) return;

    bool ok;
    QString presetName = QInputDialog::getText(this, "새 프리셋 저장", "저장할 프리셋의 이름을 입력하세요:", QLineEdit::Normal, "", &ok);

    if(ok && !presetName.isEmpty()) {
        // Controller에게 현재 MainWindow의 설정을 이 이름으로 저장해달라고 요청
        emit saveAsPresetRequested(presetName);
    }
}

void SettingsDialog::onLoadPresetClicked()
{
    if(!m_controller) return;

    QListWidgetItem* currentItem = ui->presetListWidget->currentItem();
    if(!currentItem) {
        QMessageBox::warning(this, "알림", "적용할 프리셋을 선택하세요.");
        return;
    }

    // Controller에게 선택된 프리셋을 적용해달라고 요청
    emit loadPresetRequested(currentItem->text());
    m_resultState = DialogResult::PresetLoaded; // 상태를 프리셋 로드로 설정
    QDialog::accept(); // 그냥 닫음
}

void SettingsDialog::onRenamePresetClicked()
{
    if(!m_controller) return;

    QListWidgetItem* currentItem = ui->presetListWidget->currentItem();
    if(!currentItem) {
        QMessageBox::warning(this, "알림", "이름을 바꿀 프리셋을 선택하세요.");
        return;
    }

    QString oldName = currentItem->text();
    bool ok;
    QString newName = QInputDialog::getText(this, "프리셋 이름 변경", QString("'%1'의 새 이름을 입력하세요:").arg(oldName), QLineEdit::Normal, oldName, &ok);

    if(ok && !newName.isEmpty() && oldName != newName) {
        // Controller에게 이름 변경 요청
        emit renamePresetRequested(oldName, newName);
    }
}

void SettingsDialog::onDeletePresetClicked()
{
    if(!m_controller) return;

    QListWidgetItem* currentItem = ui->presetListWidget->currentItem();
    if(!currentItem) {
        QMessageBox::warning(this, "알림", "삭제할 프리셋을 선택하세요.");
        return;
    }

    QString presetName = currentItem->text();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "삭제 확인", QString("정말로 '%1' 프리셋을 삭제하시겠습니까?").arg(presetName), QMessageBox::Yes | QMessageBox::No);

    if(reply == QMessageBox::Yes) {
        // Controller에게 삭제 요청
        emit deletePresetRequested(presetName);
    }
}

// 리스트 위젯에서 선택 항목이 변경될 때
void SettingsDialog::onPresetSelectionChanged()
{
    updateUiStates(); // 버튼 활성화 등 UI 상태 업데이트

    if(m_controller && ui->presetListWidget->currentItem()) {
        // Controller에게 선택된 프리셋의 상세 정보를 요청
        m_controller->onRequestPresetValues(ui->presetListWidget->currentItem()->text());
    }
}



// --- public slots ---
void SettingsDialog::onControllerTaskFinished(const std::expected<void, std::string>& result, const QString& successMessage)
{
    if(result) {
        // 작업 성공했을 경우
        qDebug() << "Controller task success:" <<  successMessage;
        // 성공 메시지 상태바 표시
        // 작업 성공했으므로, 최산 상태 반영
        refreshPresetList();
    } else {
        // 작업 실패했을 경우
        QMessageBox::warning(this, "작업 실패", QString::fromStdString(result.error()));
    }
}

void SettingsDialog::onPresetListChanged(const std::vector<std::string>& presetList)
{
    // 목록을 새로고침 후에도 선택 상태를 유지하기 위해 현재 선택된 항목을 기억해둠
    QString currentSelection = ui->presetListWidget->currentItem() ? ui->presetListWidget->currentItem()->text() : "";

    ui->presetListWidget->clear();
    for(const auto& name : presetList) {
        ui->presetListWidget->addItem(QString::fromStdString(name));
    }

    // 이전에 선택했던 항목이 새 목록에도 존재하면 다시 선택
    for(int i = 0; i < ui->presetListWidget->count(); ++i) {
        if(ui->presetListWidget->item(i)->text() == currentSelection) {
            ui->presetListWidget->setCurrentRow(i);
            break;
        }
    }
}

// Controller가 보내준 선택된 프리셋의 상세 값으로 오른쪽 UI를 업데이트
void SettingsDialog::onPresetValuesFetched(const QVariantMap& data)
{
    ui->previewTableWidget->clearContents();
    ui->previewTableWidget->setRowCount(0);

    // QVariantMap을 순회하며 테이블을 동적으로 채움
    for(auto it = data.constBegin(); it != data.constEnd(); ++it) {
        int currentRow = ui->previewTableWidget->rowCount();
        ui->previewTableWidget->insertRow(currentRow);

        // 0번 컬럼: 속성 이름
        QTableWidgetItem* keyItem = new QTableWidgetItem(it.key());
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
        ui->previewTableWidget->setItem(currentRow, 0, keyItem);

        // 1번 컬럼: 속성 값
        QTableWidgetItem* valueItem = new QTableWidgetItem(it.value().toString());
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        ui->previewTableWidget->setItem(currentRow, 1, valueItem);
    }
}

void SettingsDialog::onCurrentSettingsFetched(int maxDataSize, double graphWidth)
{
    ui->maxDataSizeSpinBox->setValue(maxDataSize);
    ui->graphWidthSpinBox->setValue(graphWidth);
}

// --- private 헬퍼 함수 ---
void SettingsDialog::refreshPresetList()
{
    if(m_controller)
        m_controller->onRequestPresetList();
}

void SettingsDialog::updateUiStates()
{
    bool itemSelected = ui->presetListWidget->currentItem() != nullptr;

    ui->presetPreviewGroupBox->setEnabled(itemSelected);

    // 선택된 항목이 있을 때만 불러오기, 이름 바꾸기, 삭제 버튼 활성화
    ui->loadPresetButton->setEnabled(itemSelected);
    ui->renamePresetButton->setEnabled(itemSelected);
    ui->deletePresetButton->setEnabled(itemSelected);

    if(!itemSelected) {
        ui->previewTableWidget->clearContents();
        ui->previewTableWidget->setRowCount(0);
    }
}

void SettingsDialog::accept()
{
    if(m_controller) {
        emit settingsApplied(ui->maxDataSizeSpinBox->value(), ui->graphWidthSpinBox->value());
    }
    m_resultState = DialogResult::Accepted; // 상태를 ok 눌림으로 설정
    QDialog::accept();
}
