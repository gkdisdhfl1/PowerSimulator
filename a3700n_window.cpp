#include "a3700n_window.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>

// ==========================
// 단일 행 위젯
// ==========================
class DataRowWidget : public QWidget
{
    Q_OBJECT
public:
    DataRowWidget(const QString& name, const QString& unit, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0); // 여백 없음
        mainLayout->setSpacing(4);// 라벨과 선 사이 간격

        // 1. 이름 - 값 - 단위를 담을 box
        auto rowLayout = new QHBoxLayout();

        QLabel* nameLabel = new QLabel(name, this);
        nameLabel->setMinimumWidth(60); // 이름 영역 너비 고정

        m_valueLabel = new QLabel("0.000", this);
        m_valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_valueLabel->setObjectName("valueLabel");

        QLabel* unitLabel = new QLabel(unit, this);
        unitLabel->setMinimumWidth(20); // 단위 영역 너비 고정

        rowLayout->addWidget(nameLabel);
        rowLayout->addWidget(m_valueLabel, 1); // 값 라벨이 남은 공간 모두 차지
        rowLayout->addWidget(unitLabel);

        // 2. 하단 라인
        QFrame* hLine = new QFrame(this);
        hLine->setFrameShape(QFrame::HLine);
        hLine->setFrameShadow(QFrame::Sunken);

        mainLayout->addLayout(rowLayout);
        mainLayout->addWidget(hLine);
    }

public slots:
    void setValue(double value) {
        m_valueLabel->setText(QString::number(value, 'f', 3));
    }

private:
    QLabel* m_valueLabel;
};

// ================================
// DataPage
// ================================
class DataPage : public QWidget
{
    Q_OBJECT
public:
    using Extractor = std::function<double(const OneSecondSummaryData&)>;

    DataPage(const QString& title,
             const QStringList& rowLabels,
             const QString& unit,
             const std::vector<Extractor>& extractors, QWidget* parent = nullptr)
        : QWidget(parent), m_extractors(extractors)
    {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(15, 10, 15, 10);
        layout->setSpacing(0);

        QLabel* titleLabel = new QLabel(title, this);
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(11);
        titleLabel->setFont(titleFont);
        layout->addWidget(titleLabel);
        layout->addSpacing(5);

        m_rowWidgets.reserve(rowLabels.count());
        for(int i{0}; i < rowLabels.count(); ++i) {
            DataRowWidget* row = new DataRowWidget(rowLabels[i], unit, this);
            layout->addWidget(row);
            m_rowWidgets.push_back(row); // 행 위젯 포인터 저장
        }

        layout->addStretch(); // 모든 요소를 밀착
    }
public slots:
void updateDataFromSummary(const OneSecondSummaryData& data)
    {
        for(size_t i = 0; i < m_extractors.size(); ++i) {
            if(i < m_rowWidgets.size()) {
                double value = m_extractors[i](data);
                m_rowWidgets[i]->setValue(value); // 각 행 위젯의 슬롯 호출
            }
        }
    }

private:
    std::vector<DataRowWidget*> m_rowWidgets;
    std::vector<Extractor> m_extractors;
};


// ---------------------------------------------
A3700N_Window::A3700N_Window(QWidget *parent)
    : QWidget{parent}
{
    setupUi();
}

void A3700N_Window::setupUi()
{
    // 1. 왼쪽 서브메뉴 생성
    m_submenu = new QListWidget();
    m_submenu->setObjectName("submenuList");
    m_submenu->setMaximumWidth(150);

    m_contentsStack = new QStackedWidget(this);

    createAndAddPage("RMS Voltage", {"A", "B", "C", "Average"}, "V",
                     {
                         [](const auto& d) { return d.totalVoltageRms.a; },
                         [](const auto& d) { return d.totalVoltageRms.b; },
                         [](const auto& d) { return d.totalVoltageRms.c; },
                         [](const auto& d) { return (d.totalVoltageRms.a + d.totalVoltageRms.b + d.totalVoltageRms.c) / 3.0; }
                     });

    createAndAddPage("Fund. Volt.", {"A", "B", "C", "Average"}, "V",
                     {
                      [](const auto& d) { return d.fundamentalVoltage[0].rms; },
                      [](const auto& d) { return d.fundamentalVoltage[1].rms; },
                      [](const auto& d) { return d.fundamentalVoltage[2].rms; },
                      [](const auto& d) { return (d.fundamentalVoltage[0].rms + d.fundamentalVoltage[1].rms + d.fundamentalVoltage[2].rms) / 3.0; }
                      });

    createAndAddPage("Total Harmonic Distortion", {"A", "B", "C"}, "%",
                     {
                      [](const auto& d) { return d.voltageThd.a; },
                      [](const auto& d) { return d.voltageThd.b; },
                      [](const auto& d) { return d.voltageThd.c; }
                      });

    createAndAddPage("Frequency", {"Frequency"}, "Hz",
                     {
                         [](const auto& d) { return d.frequency; }
                     });

    createAndAddPage("Residual Voltage", {"RMS", "Fund."}, "V",
                     {
                         [](const auto& d) { return d.residualVoltageRms; },
                         [](const auto& d) { return 0.0; }
                     });

    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_submenu);
    mainLayout->addWidget(m_contentsStack, 1);

    connect(m_submenu, &QListWidget::currentRowChanged, m_contentsStack, &QStackedWidget::setCurrentIndex);
    m_submenu->setCurrentRow(0);
}

void A3700N_Window::createAndAddPage(const QString& title,
                      const QStringList& rowLabels,
                      const QString& unit,
                      const std::vector<std::function<double(const OneSecondSummaryData&)>>& extractors)
{
    DataPage* page = new DataPage(title, rowLabels, unit, extractors);

    connect(this, &A3700N_Window::dataUpdated, page, &DataPage::updateDataFromSummary);

    m_contentsStack->addWidget(page);
    m_submenu->addItem(rowLabels.count() > 1 ? title : rowLabels[0]); // 메뉴 이름 설정
}

void A3700N_Window::updateData(const OneSecondSummaryData& data)
{
    emit dataUpdated(data);
}

#include "a3700n_window.moc"
