 #include "harmonics_dialog.h"

#include "harmonic_editor.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>

HarmonicsDialog::HarmonicsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Harmonics Settings");
    resize(600, 500);

    auto* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget();

    // 전압 고조파 에디터
    m_voltageEditor = new HarmonicEditor("V");
    m_tabWidget->addTab(m_voltageEditor, "Voltage");

    // 전류 고조파 에디터
    m_currentEditor = new HarmonicEditor("A");
    m_tabWidget->addTab(m_currentEditor, "Current");

    mainLayout->addWidget(m_tabWidget);

    // 하단 닫기 버튼
    auto* closeBtn = new QPushButton("Close");
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);

    // 시그널 연결
    connect(m_voltageEditor, &HarmonicEditor::harmonicsChanged, this, &HarmonicsDialog::voltageHarmonicsChanged);
    connect(m_currentEditor, &HarmonicEditor::harmonicsChanged, this, &HarmonicsDialog::currentHarmonicsChanged);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void HarmonicsDialog::setHarmonics(const HarmonicList& voltageHarmonics, const HarmonicList& currentHarmonics)
{
    // 각 에디터에 초기 데이터 설정
    m_voltageEditor->setHarmonics(voltageHarmonics);
    m_currentEditor->setHarmonics(currentHarmonics);
}
