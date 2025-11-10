#include "phasor_view.h"
#include "analysis_utils.h"
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
    , m_voltageLabel(new QLabel("전압:", this))
    , m_currentLabel(new QLabel("전류:", this))
    , m_voltageInfoLabel(new QLabel(this))
    , m_currentInfoLabel(new QLabel(this))
{
    // 가시성 상태 초기화
    m_phasorIsVisible.fill(false);
    m_phasorIsVisible[0] = true; // V(A)
    m_phasorIsVisible[3] = true; // I(A)

    m_infoLayout = new QGridLayout();
    m_infoLayout->addWidget(m_voltageLabel, 0, 0);
    m_infoLayout->addWidget(m_voltageInfoLabel, 0, 1);
    m_infoLayout->addWidget(m_currentLabel, 1, 0);
    m_infoLayout->addWidget(m_currentInfoLabel, 1, 1);
    m_infoLayout->setColumnStretch(1, 1);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addStretch(); // 그림 영역이 나머지 공간을 차지
    mainLayout->addLayout(m_infoLayout);

    // 위젯 최소 크기 설정
    setMinimumSize(350, 250);
    setInfoLabelVisibility(true);
}

void PhasorView::updateData(const std::array<HarmonicAnalysisResult, 3>& fundamentalVoltage,
                            const std::array<HarmonicAnalysisResult, 3>& fundamentalCurrent,
                            const std::vector<HarmonicAnalysisResult>& voltageHarmonics,
                            const std::vector<HarmonicAnalysisResult>& currentHarmonics)
{
    // --- 기본파 정보 계산 ---

    for(int i{0}; i < 3; ++i) {
        // 전압
        const auto& v_fund = fundamentalVoltage[i];
        if (v_fund.order > 0) {
            m_fundVoltage[i].components = QPointF(v_fund.phasorX, v_fund.phasorY);
            m_fundVoltage[i].magnitude = v_fund.rms;
            m_fundVoltage[i].phaseDegrees = utils::radiansToDegrees(v_fund.phase);
        } else { m_fundVoltage[i] = PhasorInfo(); }

        // 전류
        const auto& i_fund = fundamentalCurrent[i];
        if (i_fund.order > 0) {
            m_fundCurrent[i].components = QPointF(i_fund.phasorX, i_fund.phasorY);
            m_fundCurrent[i].magnitude = i_fund.rms;
            m_fundCurrent[i].phaseDegrees = utils::radiansToDegrees(i_fund.phase);
        } else { m_fundCurrent[i] = PhasorInfo(); }
    }

    // --- 고조파 정보 계산 ---
    const auto* v_harm = AnalysisUtils::getDominantHarmonic(voltageHarmonics);
    const auto* i_harm = AnalysisUtils::getDominantHarmonic(currentHarmonics);

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
                                    .arg(m_fundVoltage[0].magnitude, 0, 'f', 2)
                                    .arg(m_fundVoltage[0].phaseDegrees, 0, 'f', 1));

    m_currentInfoLabel->setText(QString("고조파: %1 A, %2°\n기본파: %3 A, %4°")
                                    .arg(m_harmonicCurrent.magnitude, 0, 'f', 2)
                                    .arg(m_harmonicCurrent.phaseDegrees, 0, 'f', 1)
                                    .arg(m_fundCurrent[0].magnitude, 0, 'f', 2)
                                    .arg(m_fundCurrent[0].phaseDegrees, 0, 'f', 1));

    update();
}

void PhasorView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 컨트롤 영역의 높이를 제외하고 컨텍스트 생성
    const int topMargin = m_infoLayout->sizeHint().height();
    DrawingContext ctx(rect(), topMargin);

    painter.translate(ctx.origin);

    // 가이드 라인 그리기
    drawGuideLines(painter, ctx);

    // --- 페이저 그리기 ---
    // 3상 기본파 페이저 그리기

    // 최대값 찾기
    double maxVoltageMagnitude = 0.0;
    double maxCurrentMagnitude = 0.0;
    for(int i{0}; i < 3; ++i) {
        if(m_phasorIsVisible[i])
            maxVoltageMagnitude = std::max(maxVoltageMagnitude, m_fundVoltage[i].magnitude);
        if(m_phasorIsVisible[i + 3])
            maxCurrentMagnitude = std::max(maxCurrentMagnitude, m_fundCurrent[i].magnitude);
    }
    if(m_phasorIsVisible[6]) {
        maxVoltageMagnitude = std::max(maxVoltageMagnitude, m_harmonicVoltage.magnitude);
    }
    if(m_phasorIsVisible[7]) {
        maxCurrentMagnitude = std::max(maxCurrentMagnitude, m_harmonicCurrent.magnitude);
    }


    // 전압 페이저 그리기
    if(maxVoltageMagnitude > 1e-6) {
        for(int i{0}; i < 3; ++i) {
            if(m_phasorIsVisible[i]) {
                double ratio = m_fundVoltage[i].magnitude / maxVoltageMagnitude;
                drawPhasor(painter, m_fundVoltage[i], config::View::PhaseColors::Voltage[i], getPhasorDisplayLength(ratio, ctx, true));
            }
        }
        // 고조파 전압
        if(m_phasorIsVisible[6]) {
            double ratio = m_harmonicVoltage.magnitude / maxVoltageMagnitude;
            drawPhasor(painter, m_harmonicVoltage, Qt::gray, getPhasorDisplayLength(ratio, ctx, true));
        }
    }

    // 전류 페이저 그리기
    if(maxCurrentMagnitude > 1e-6) {
        for(int i{0}; i < 3; ++i) {
            if(m_phasorIsVisible[i + 3]) {
                double ratio  = m_fundCurrent[i].magnitude / maxCurrentMagnitude;
                drawPhasor(painter, m_fundCurrent[i], config::View::PhaseColors::Current[i], getPhasorDisplayLength(ratio, ctx, false));
            }
        }
        // 고조파 전류
        if(m_phasorIsVisible[7]) {
            double ratio = m_harmonicCurrent.magnitude / maxCurrentMagnitude;
            drawPhasor(painter, m_harmonicCurrent, Qt::green, getPhasorDisplayLength(ratio, ctx, false));
        }
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

double PhasorView::getPhasorDisplayLength(double ratio, const DrawingContext& ctx, bool isVoltage) const
{
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

void PhasorView::onVisibilityChanged(int type, bool isVisible)
{
    qDebug() << "onVisibilityChanged(" << type << ", " << isVisible << ")";
    if(type >= 0 && type < m_phasorIsVisible.size()) {
        m_phasorIsVisible[type] = isVisible;
        update();
    }
}

void PhasorView::setInfoLabelVisibility(bool visible)
{
    m_voltageLabel->setVisible(visible);
    m_currentLabel->setVisible(visible);
    m_voltageInfoLabel->setVisible(visible);
    m_currentInfoLabel->setVisible(visible);
}
