#include "settings_dialog.h"
#include "settings_ui_controller.h"
#include "config.h"
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QHeaderView>
#include <QFormLayout>

SettingsDialog::SettingsDialog(SettingsUiController* controller ,QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_resultState(DialogResult::Cancled)
{
    setupUi();
    m_maxDataSizeSpinBox->setRange(config::Simulation::MinDataSize, config::Simulation::MaxDataSize);

    m_graphWidthSpinBox->setSuffix(" s");
    m_graphWidthSpinBox->setRange(config::View::GraphWidth::Min, config::View::GraphWidth::Max);
    m_graphWidthSpinBox->setValue(config::View::GraphWidth::Default);

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
    connect(m_newPresetButton, &QPushButton::clicked, this, &SettingsDialog::onNewPresetClicked);
    connect(m_loadPresetButton, &QPushButton::clicked, this, &SettingsDialog::onLoadPresetClicked);
    connect(m_renamePresetButton, &QPushButton::clicked, this, &SettingsDialog::onRenamePresetClicked);
    connect(m_deletePresetButton, &QPushButton::clicked, this, &SettingsDialog::onDeletePresetClicked);

    // 리스트 위젯의 선택이 변경될 때의 시그널을 슬롯에 연결
    connect(m_presetListWidget, &QListWidget::currentItemChanged, this, &SettingsDialog::onPresetSelectionChanged);

    // 확인/취소 버튼을 QDialog의 기본 슬롯인 accept/reject에 연결
    connect(m_okButton, &QPushButton::clicked, this, &SettingsDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::reject);
}

SettingsDialog::~SettingsDialog()
{
}

// ------- public ----------
void SettingsDialog::setupUi()
{
    setWindowTitle("SettingDialog");

    // 1. 왼쪽 패널 (프리셋 목록)
    auto presetLabel = new QLabel("프리셋 목록");
    m_presetListWidget = new QListWidget();

    auto presetButtonsLayout = new QHBoxLayout();
    m_newPresetButton = new QPushButton("새로 만들기");
    m_loadPresetButton = new QPushButton("불러오기");
    m_deletePresetButton = new QPushButton("삭제");
    m_renamePresetButton = new QPushButton("이름 변경");
    presetButtonsLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    presetButtonsLayout->addWidget(m_newPresetButton);
    presetButtonsLayout->addWidget(m_loadPresetButton);
    presetButtonsLayout->addWidget(m_deletePresetButton);
    presetButtonsLayout->addWidget(m_renamePresetButton);

    auto leftLayout = new QVBoxLayout();
    leftLayout->addWidget(presetLabel);
    leftLayout->addWidget(m_presetListWidget);
    leftLayout->addLayout(presetButtonsLayout);

    // 2. 중간 패널 (프리셋 미리보기)
    m_previewTableWidget = new QTableWidget();
    m_previewTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_previewTableWidget->setColumnCount(2);
    m_previewTableWidget->horizontalHeader()->setVisible(false);
    m_previewTableWidget->horizontalHeader()->setStretchLastSection(true);
    m_previewTableWidget->verticalHeader()->setVisible(false);

    auto previewLayout = new QGridLayout();
    previewLayout->addWidget(m_previewTableWidget, 0, 0);

    m_previewGroupBox = new QGroupBox("프리셋 미리보기");
    m_previewGroupBox->setLayout(previewLayout);

    // --- 3. 오른쪽 패널 (상세 설정 및 확인/취소) ---
    auto detailsLabel = new QLabel("저장 크기");
    m_maxDataSizeSpinBox = new QSpinBox();
    m_maxDataSizeSpinBox->setRange(config::Simulation::MinDataSize, config::Simulation::MaxDataSize);
    m_maxDataSizeSpinBox->setSingleStep(100);

    auto graphWidthLabel = new QLabel("그래프 폭");
    m_graphWidthSpinBox = new QDoubleSpinBox();
    m_graphWidthSpinBox->setRange(config::View::GraphWidth::Min, config::View::GraphWidth::Max);
    m_graphWidthSpinBox->setSingleStep(0.1);
    m_graphWidthSpinBox->setDecimals(2);

    auto formLayout = new QFormLayout();
    formLayout->addRow(detailsLabel, m_maxDataSizeSpinBox);
    formLayout->addRow(graphWidthLabel, m_graphWidthSpinBox);

    auto detailsGroupBox = new QGroupBox("상세 설정");
    detailsGroupBox->setLayout(formLayout);

    m_okButton = new QPushButton("Ok");
    m_okButton->setDefault(true);
    m_cancelButton = new QPushButton("Cancel");

    auto dialogButtonsLayout = new QHBoxLayout();
    dialogButtonsLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    dialogButtonsLayout->addWidget(m_okButton);
    dialogButtonsLayout->addWidget(m_cancelButton);

    auto rightLayout = new QVBoxLayout();
    rightLayout->addWidget(detailsGroupBox);
    rightLayout->stretch(1);
    rightLayout->addLayout(dialogButtonsLayout);

    // 4. 전체 다이얼로그 레이아웃
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addWidget(m_previewGroupBox, 3);
    mainLayout->addLayout(rightLayout, 2);
}

int SettingsDialog::openWithValues(const SimulationEngine* params)
{
    m_maxDataSizeSpinBox->setValue(params->m_maxDataSize.value());
    m_graphWidthSpinBox->setValue(params->m_graphWidthSec.value());
    refreshPresetList();
    updateUiStates();

    m_resultState = DialogResult::Cancled;
    return exec();
}

int SettingsDialog::getMaxSize() const
{
    return m_maxDataSizeSpinBox->value();
}
double SettingsDialog::getGraphWidth() const
{
    return m_graphWidthSpinBox->value();
}
SettingsDialog::DialogResult SettingsDialog::getResultState() const
{
    return m_resultState;
}
// ---------------------------

// --- public slots ---
void SettingsDialog::onControllerTaskFinished(const std::expected<void, std::string>& result, const QString& successMessage)
{
    if(result) {
        // 작업 성공했을 경우
        qDebug() << "Controller task success:" <<  successMessage;
        // 성공 메시지 상태바 표시
        // 작업 성공했으므로, 최신 상태 반영
        refreshPresetList();
    } else {
        // 작업 실패했을 경우
        QMessageBox::warning(this, "작업 실패", QString::fromStdString(result.error()));
    }
}

void SettingsDialog::onPresetListChanged(const std::vector<std::string>& presetList)
{
    // 목록을 새로고침 후에도 선택 상태를 유지하기 위해 현재 선택된 항목을 기억해둠
    QString currentSelection = m_presetListWidget->currentItem() ? m_presetListWidget->currentItem()->text() : "";

    m_presetListWidget->clear();
    for(const auto& name : presetList) {
        m_presetListWidget->addItem(QString::fromStdString(name));
    }

    // 이전에 선택했던 항목이 새 목록에도 존재하면 다시 선택
    for(int i = 0; i < m_presetListWidget->count(); ++i) {
        if(m_presetListWidget->item(i)->text() == currentSelection) {
            m_presetListWidget->setCurrentRow(i);
            break;
        }
    }
}

// Controller가 보내준 선택된 프리셋의 상세 값으로 오른쪽 UI를 업데이트
void SettingsDialog::onPresetValuesFetched(const QVariantMap& data)
{
    m_previewTableWidget->clearContents();
    m_previewTableWidget->setRowCount(0);

    // QVariantMap을 순회하며 테이블을 동적으로 채움
    for(auto it = data.constBegin(); it != data.constEnd(); ++it) {
        int currentRow = m_previewTableWidget->rowCount();
        m_previewTableWidget->insertRow(currentRow);

        // 0번 컬럼: 속성 이름
        QTableWidgetItem* keyItem = new QTableWidgetItem(it.key());
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);
        m_previewTableWidget->setItem(currentRow, 0, keyItem);

        // 1번 컬럼: 속성 값
        QTableWidgetItem* valueItem = new QTableWidgetItem(it.value().toString());
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        m_previewTableWidget->setItem(currentRow, 1, valueItem);
    }
}
// ---------------------

// ----- private 슬롯 구현 -----
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

    QListWidgetItem* currentItem = m_presetListWidget->currentItem();
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

    QListWidgetItem* currentItem = m_presetListWidget->currentItem();
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

    QListWidgetItem* currentItem = m_presetListWidget->currentItem();
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

    if(m_controller && m_presetListWidget->currentItem()) {
        // Controller에게 선택된 프리셋의 상세 정보를 요청
        m_controller->onRequestPresetValues(m_presetListWidget->currentItem()->text());
    }
}
// -----------------------------



// --- private 헬퍼 함수 ---
void SettingsDialog::refreshPresetList()
{
    if(m_controller)
        m_controller->onRequestPresetList();
}

void SettingsDialog::updateUiStates()
{
    bool itemSelected = m_presetListWidget->currentItem() != nullptr;

    m_previewGroupBox->setEnabled(itemSelected);

    // 선택된 항목이 있을 때만 불러오기, 이름 바꾸기, 삭제 버튼 활성화
    m_loadPresetButton->setEnabled(itemSelected);
    m_renamePresetButton->setEnabled(itemSelected);
    m_deletePresetButton->setEnabled(itemSelected);

    if(!itemSelected) {
        m_previewTableWidget->clearContents();
        m_previewTableWidget->setRowCount(0);
    }
}

void SettingsDialog::accept()
{
    if(m_controller) {
        emit settingsApplied(m_maxDataSizeSpinBox->value(), m_graphWidthSpinBox->value());
    }
    m_resultState = DialogResult::Accepted; // 상태를 ok 눌림으로 설정
    QDialog::accept();
}
