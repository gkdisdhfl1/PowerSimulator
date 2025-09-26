#include "pid_controller.h"
#include <algorithm>

void PIDController::setCoefficients(const Coefficients& coeffs)
{
    m_coeffs = coeffs;
}
PIDController::Coefficients PIDController::getCoefficients()
{
    return m_coeffs;
}

void PIDController::setLimits(double integralLimit, double outputLimit)
{
    m_integralLimit = integralLimit;
    m_outputLimit = outputLimit;
}

double PIDController::process(double error)
{
    if(m_integralActivationThreshold > 0.0 && std::abs(error) > m_integralActivationThreshold) {
        m_integral = 0.0;
    } else {
        m_integral += error;
    }
    // Integral term
    m_integral = std::clamp(m_integral, -m_integralLimit, m_integralLimit);

    // Derivative term
    double derivative = error - m_previousError;
    m_previousError = error;

    // PID output
    double output = (m_coeffs.Kp * error) + (m_coeffs.Ki * m_integral) + (m_coeffs.Kd * derivative);

    return std::clamp(output, -m_outputLimit, m_outputLimit);
}

void PIDController::reset()
{
    m_integral = 0.0;
    m_previousError = 0.0;
}

void PIDController::setIntegralActivationThreshold(double threshold)
{
    m_integralActivationThreshold = threshold;
}
