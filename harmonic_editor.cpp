#include "harmonic_editor.h"

#include "value_control_widget.h"

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>

class HarmonicItem : public QWidget {
    Q_OBJECT

public:
    explicit HarmonicItem(const HarmonicComponent& data, QWidget* parent = nullptr) : QWidget(parent) {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->addSpacing(15);

        // 차수 - integer 타입
        m_orderControl = new ValueControlWidget();
        m_orderControl->setRange(2, 50);
        m_orderControl->setDataType(ValueControlWidget::DataType::Integer);
        m_orderControl->setValue(data.order);

        auto* orderBox = new QVBoxLayout();
        orderBox->addWidget(new QLabel("Order"), 0, Qt::AlignCenter);
        orderBox->addWidget(m_orderControl);
        layout->addLayout(orderBox);

        // 크기 - double 타입
        m_magControl = new ValueControlWidget();
        m_magControl->setRange(0.0, 500.0);
        m_magControl->setSuffix("V");
        m_magControl->setValue(data.magnitude);

       auto* magBox = new QVBoxLayout();
       magBox->addWidget(new QLabel("Magnitude"), 0, Qt::AlignCenter);
       magBox->addWidget(m_magControl);
       layout->addLayout(magBox);

       // 위상 - double 타입
       m_phaseControl = new ValueControlWidget();
       m_phaseControl->setRange(-360.0, 360.0);
       m_phaseControl->setSuffix("°");
       m_phaseControl->setValue(data.phase);

       auto* phaseBox = new QVBoxLayout();
       phaseBox->addWidget(new QLabel("Phase"), 0, Qt::AlignCenter);
       phaseBox->addWidget(m_phaseControl);
       layout->addLayout(phaseBox);

       // 삭제 버튼
       auto* delBtn = new QPushButton("❌");
       delBtn->setFixedSize(30, 30);
       delBtn->setFlat(true);
       layout->addWidget(delBtn);

       // 시그널 연결
       connect(m_orderControl, &ValueControlWidget::intValueChanged, this, &HarmonicItem::changed);
       connect(m_magControl, &ValueControlWidget::valueChanged, this, &HarmonicItem::changed);
       connect(m_phaseControl, &ValueControlWidget::valueChanged, this, &HarmonicItem::changed);
       connect(delBtn, &QPushButton::clicked, this, &HarmonicItem::deleteRequested);
    }

    HarmonicComponent getData() const {
        return {
            static_cast<int>(m_orderControl->value()),
            m_magControl->value(),
            m_phaseControl->value()
        };
    }

signals:
    void changed();
    void deleteRequested();

private:
    ValueControlWidget* m_orderControl;
    ValueControlWidget* m_magControl;
    ValueControlWidget* m_phaseControl;
};

HarmonicEditor::HarmonicEditor(QWidget *parent)
    : QWidget{parent}
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* addBtn = new QPushButton("+ Add Harmonic");
    addBtn->setFixedWidth(120);
    mainLayout->addWidget(addBtn, 0, Qt::AlignRight);

    // 스크롤 영역
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* container = new QWidget();
    m_listLayout = new QVBoxLayout(container);
    m_listLayout->setAlignment(Qt::AlignTop);
    m_listLayout->setSpacing(5);

    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);

    connect(addBtn, &QPushButton::clicked, this, &HarmonicEditor::onAddClicked);
}

void HarmonicEditor::setHarmonics(const HarmonicList& harmonics)
{
    // 기존 아이템 모두 삭제
    QLayoutItem* item;
    while((item = m_listLayout->takeAt(0)) != nullptr) {
        if(item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // 새로운 아이템 추가
    for(const auto& h : harmonics) {
        addRow(h);
    }
}

HarmonicList HarmonicEditor::getHarmonics() const
{
    HarmonicList list;
    for(int i = 0; i < m_listLayout->count(); ++i) {
        auto* widget = qobject_cast<HarmonicItem*>(m_listLayout->itemAt(i)->widget());
        if(widget) {
            list.push_back(widget->getData());
        }
    }
    return list;
}

void HarmonicEditor::onAddClicked()
{
    addRow({2, 10.0, 0.0});
    emit harmonicsChanged(getHarmonics());
}

void HarmonicEditor::onItemChanged()
{
    emit harmonicsChanged(getHarmonics());
}

void HarmonicEditor::onItemDeleted()
{
    auto* itemWidget = qobject_cast<HarmonicItem*>(sender());
    if(itemWidget) {
        m_listLayout->removeWidget(itemWidget);
        itemWidget->deleteLater();
        // 삭제 후 다음 이벤트 루프에서 데이터 갱신 알림
        QTimer::singleShot(0, this, [this](){ emit harmonicsChanged(getHarmonics()); });
    }
}

void HarmonicEditor::addRow(const HarmonicComponent& data)
{
    auto* item = new HarmonicItem(data);
    m_listLayout->addWidget(item);

    connect(item, &HarmonicItem::changed, this, &HarmonicEditor::onItemChanged);
    connect(item, &HarmonicItem::deleteRequested, this, &HarmonicEditor::onItemDeleted);
}

#include "harmonic_editor.moc"
