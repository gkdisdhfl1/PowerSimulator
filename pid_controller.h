#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

class PIDController
{
public:
    struct Coefficients {
        double Kp;
        double Ki;
        double Kd;
    };

    void setCoefficients(const Coefficients& coeffs);
    Coefficients getCoefficients();
    void setLimits(double integralLimit, double outputLimit);
    double process(double error);
    void reset();
    void setIntegralActivationThreshold(double threshold);

private:
    Coefficients m_coeffs;
    double m_integral = 0.0;
    double m_previousError = 0.0;
    double m_integralLimit = 10.0;
    double m_outputLimit = 1.0;
    double m_integralActivationThreshold = 0.0;
};

#endif // PID_CONTROLLER_H
