#include "phasor_view.h"
#include "AnalysisUtils.h"
#include "config.h"

#include <QCheckBox>
#include <QVBoxLayout>
#include <QPainter>
#include <QLabel>
#include <QGroupBox>

namespace {
    // 페이저를 그릴 때 사용하는 상수들
    constexpr int AXIS_PADDING = 10;
    constexpr double ARROW_HEAD_LENGTH = 5.0;
    constexpr double ARROW_HEAD_ANGLE = std::numbers::pi / 6.0; // 30도
    constexpr double VOLTAGE_MAX_SCALE = 0.95;
    constexpr double CURRENT_BASE_SCALE = 0.60;
}

DrawingContext::DrawingContext(const QRect& widgetRect, int topMargin)
{
    const QRect drawingRect = widgetRect.adjusted(0, 0, 0, -topMargin);
    origin = drawingRect.center();
    maxRadius = std::min(drawingRect.width(), drawingRect.height()) / 2.0 - AXIS_PADDING;

    voltageRadius = maxRadius * VOLTAGE_MAX_SCALE;
    currentBaseRadius = maxRadius * CURRENT_BASE_SCALE;
    voltageAnnulusHeight = voltageRadius - currentBaseRadius;
}

PhasorView::PhasorView(QWidget *parent)
    : QWidget{parent}
    , m_controlContainer(new QWidget(this))
    , m_totalVoltageCheck(new QCheckBox("고조파 V", this))
    , m_totalCurrentCheck(new QCheckBox("고조파 I", this))
    , m_fundVoltageCheck(new QCheckBox("기본파 V", this))
    , m_fundCurrentCheck(new QCheckBox("기본파 I", this))
    , m_voltageInfoLabel(new QLabel(this))
    , m_currentInfoLabel(new QLabel(this))
{
    // 체크 박스 기본값 설정
    m_totalVoltageCheck->setChecked(false);
    m_totalCurrentCheck->setChecked(false);
    m_fundVoltageCheck->setChecked(true);
    m_fundCurrentCheck->setChecked(true);

    // 체크박스가 변경되면 위젯을 다시 그리도록 함
    connect(m_totalVoltageCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_totalCurrentCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_fundVoltageCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));
    connect(m_fundCurrentCheck, &QCheckBox::checkStateChanged, this, QOverload<>::of(&PhasorView::update));

    // 레이아웃 설정
    auto controlLayout = new QGridLayout(m_controlContainer);
    controlLayout->addWidget(m_totalVoltageCheck, 0, 0);
    controlLayout->addWidget(m_totalCurrentCheck, 0, 1);
    controlLayout->addWidget(m_fundVoltageCheck, 1, 0);
    controlLayout->addWidget(m_fundCurrentCheck, 1, 1);

    controlLayout->addWidget(m_voltageInfoLabel, 0, 2);
    controlLayout->addWidget(m_currentInfoLabel, 1, 2);

    controlLayout->setColumnStretch(3, 1);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addStretch(); // 그림 영역이 나머지 공간을 차지
    mainLayout->addWidget(m_controlContainer);

    // 위젯 최소 크기 설정
    setMinimumSize(350, 250);
}

void PhasorView::updateData(const std::deque<MeasuredData>& data)
{
    if(data.empty()) {
        m_harmonicVoltage = PhasorInfo();
        m_harmonicCurrent = PhasorInfo();
        m_fundamentalVoltage = PhasorInfo();
        m_fundamentalCurrent = PhasorInfo();
        m_voltageInfoLabel->clear();
        m_currentInfoLabel->clear();
    }

    const auto& latestData = data.back();

    // --- 기본파 정보 계산 ---
    const auto* v_fund = AnalysisUtils::getHarmonicComponent(latestData.voltageHarmonics, 1);
    const auto* i_fund = AnalysisUtils::getHarmonicComponent(latestData.currentHarmonics, 1);

    if (v_fund) {
        m_fundamentalVoltage.components = QPointF(v_fund->phasorX, v_fund->phasorY);
        m_fundamentalVoltage.magnitude = v_fund->rms;
        m_fundamentalVoltage.phaseDegrees = utils::radiansToDegrees(v_fund->phase);
    } else { m_fundamentalVoltage = PhasorInfo(); }

    if (i_fund) {
        m_fundamentalCurrent.components = QPointF(i_fund->phasorX, i_fund->phasorY);
        m_fundamentalCurrent.magnitude = i_fund->rms;
        m_fundamentalCurrent.phaseDegrees = utils::radiansToDegrees(i_fund->phase);
    } else { m_fundamentalCurrent = PhasorInfo(); }

    // --- 고조파 정보 계산 ---
    const auto* v_harm = AnalysisUtils::getDominantHarmonic(latestData.voltageHarmonics);
    const auto* i_harm = AnalysisUtils::getDominantHarmonic(latestData.currentHarmonics);

    if(v_harm) {
        m_harmonicVoltage.components = QPointF(v_harm->phasorX, v_harm->phasorY);
        m_harmonicVoltage.magnitude = v_harm->rms;
        m_harmonicVoltage.phaseDegrees = utils::radiansToDegrees(v_harm->phase);
    } else {
        m_harmonicVoltage = PhasorInfo();
    }

    if(i_harm) {
        m_harmonicCurrent.components = QPointF(i_harm->phasorX, i_harm->phasorY);
        m_harmonicCurrent.magnitude = i_harm->rms;
        m_harmonicCurrent.phaseDegrees = utils::radiansToDegrees(i_harm->phase);
    } else {
        m_harmonicCurrent = PhasorInfo();
    }

    // --- 라벨 업데이트 ---
    m_voltageInfoLabel->setText(QString("고조파: %1 V, %2°\n기본파: %3 V, %4°")
                                    .arg(m_harmonicVoltage.magnitude, 0, 'f', 2)
                                    .arg(m_harmonicVoltage.phaseDegrees, 0, 'f', 1)
                                    .arg(m_fundamentalVoltage.magnitude, 0, 'f', 2)
                                    .arg(m_fundamentalVoltage.phaseDegrees, 0, 'f', 1));

    m_currentInfoLabel->setText(QString("고조파: %1 A, %2°\n기본파: %3 A, %4°")
                                    .arg(m_harmonicCurrent.magnitude, 0, 'f', 2)
                                    .arg(m_harmonicCurrent.phaseDegrees, 0, 'f', 1)
                                    .arg(m_fundamentalCurrent.magnitude, 0, 'f', 2)
                                    .arg(m_fundamentalCurrent.phaseDegrees, 0, 'f', 1));

    update();
}

void PhasorView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 컨트롤 영역의 높이를 제외하고 컨텍스트 생성
    const int topMargin = m_controlContainer->height();
    DrawingContext ctx(rect(), topMargin);

    painter.translate(ctx.origin);

    // 가이드 라인 그리기
    drawGuideLines(painter, ctx);

    // 페이저 그리기

    // 고조파 페이저
    if(m_totalVoltageCheck->isChecked()) {
        drawPhasor(painter, m_harmonicVoltage, Qt::cyan, getPhasorDisplayLength(m_harmonicVoltage.magnitude, config::Source::Amplitude::Max, ctx, true));
    }
    if(m_totalCurrentCheck->isChecked()) {
        // qDebug() << "currentDisplayLength: " << currentDisplayLength;
        drawPhasor(painter, m_harmonicCurrent, Qt::magenta, getPhasorDisplayLength(m_harmonicCurrent.magnitude, config::Source::Current::MaxAmplitude, ctx, false));
    }
    // 기본파 페이저
    if(m_fundVoltageCheck->isChecked()) {
        drawPhasor(painter, m_fundamentalVoltage, Qt::blue, getPhasorDisplayLength(m_fundamentalVoltage.magnitude, config::Source::Amplitude::Max, ctx, true));
    }
    if(m_fundCurrentCheck->isChecked()) {
        // qDebug() << "currentDisplayLength: " << currentDisplayLength;
        drawPhasor(painter, m_fundamentalCurrent, Qt::red, getPhasorDisplayLength(m_fundamentalCurrent.magnitude, config::Source::Current::MaxAmplitude, ctx, false));
    }
}

void PhasorView::drawGuideLines(QPainter& painter, const DrawingContext& ctx) const
{
    // 좌표축
    painter.setPen(Qt::gray);
    painter.drawLine(-ctx.maxRadius, 0, ctx.maxRadius, 0);
    painter.drawLine(0, -ctx.maxRadius, 0, ctx.maxRadius);

    // 가이드라인 원
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    painter.drawEllipse(QPointF(0, 0), ctx.voltageRadius, ctx.voltageRadius);
    painter.drawEllipse(QPointF(0, 0), ctx.currentBaseRadius, ctx.currentBaseRadius);
}

double PhasorView::getPhasorDisplayLength(double magnitude, double maxMagnitude, const DrawingContext& ctx, bool isVoltage) const
{
    if(maxMagnitude < 1e-6)
        return 0.0;

    double ratio = log10(1 + magnitude) / log10(1 + maxMagnitude);
    ratio = std::clamp(ratio, 0.0, 1.0);

    if(isVoltage) {
        return ctx.currentBaseRadius + (ctx.voltageAnnulusHeight * ratio);
    } else {
        return ctx.currentBaseRadius * ratio;
    }
}

void PhasorView::drawPhasor(QPainter& painter, const PhasorInfo& phasor, const QColor& color, double radius)
{
    if(phasor.magnitude < 1e-6) // 크기가 0에 가까우면
        return;

    // Y축은 위쪽이 음수이므로 부호 반전
    const QPointF endPoint(phasor.components.x() / phasor.magnitude* radius, -phasor.components.y() / phasor.magnitude * radius);

    QPen pen(color);
    pen.setWidth(2);
    painter.setPen(pen);

    // 벡터 라인 그리기
    painter.drawLine(QPointF(0, 0), endPoint);

    // 벡터 화살표 그리기
    const double angle = std::atan2(endPoint.y(), endPoint.x());
    const QPointF arrowP1 = endPoint - QPointF(std::cos(angle + ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH, std::sin(angle + ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH);
    const QPointF arrowP2 = endPoint - QPointF(std::cos(angle - ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH, std::sin(angle - ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH);

    painter.drawLine(endPoint, arrowP1);
    painter.drawLine(endPoint, arrowP2);
}
