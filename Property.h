#ifndef PROPERTY_H
#define PROPERTY_H

#include <QObject>
#include "shared_data_types.h"
#include <QDebug>

// 템플릿이 아닌 부모 클래스에 모든 타입의 시그널/슬롯을 미리 정의
class PropertySignals : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

public slots:
    // setValue를 슬롯으로 만들기 위해 virtual 선언
    virtual void setValue(const int&){}
    virtual void setValue(const double&){}
    virtual void setValue(const bool&){}
    virtual void setValue(const UpdateMode&){}
    virtual void setValue(const HarmonicComponent&){}

signals:
    // 모든 필요한 타입에 대한 valueChanged 시그널을 오버로딩하여 선언
    void valueChanged(const int& newValue);
    void valueChanged(const double& newValue);
    void valueChanged(const bool& newValue);
    void valueChanged(const UpdateMode& newValue);
    void valueChanged(const HarmonicComponent& newValue);
};

template <typename T>
class Property : public PropertySignals
{
public:
    // 생성자에서 초기값 설정
    Property(T initialValue, QObject* parent = nullptr)
        : PropertySignals(parent), m_value(initialValue) {}

    // 현재 값을 반환하는 Getter
    T value() const
    {
        return m_value;
    }

    // 값을 변경하는 Setter
    void setValue(const T& newValue) override
    {
        // qDebug() << "Property::setValue: 들어온 값: " << newValue;
        if(m_value != newValue) {
            m_value = newValue;
            // qDebug() << "Property::setValue: 값이 변경됨: " << m_value;
            emit valueChanged(newValue); // T 타입에 맞는 오버로딩된 시그널을 자동으로 호출
        }
    }

private:
    T m_value;
};


#endif // PROPERTY_H
