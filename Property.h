#ifndef PROPERTY_H
#define PROPERTY_H

#include <QObject>
#include "shared_data_types.h"
#include <QDebug>
#include <QVariant>

// 템플릿이 아닌 부모 클래스에 모든 타입의 시그널/슬롯을 미리 정의
class PropertySignals : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    // 다형성을 위한 Getter/Setter 인터페이스
    virtual QVariant getVariantValue() const = 0;
    virtual void setVariantValue(const QVariant& val) = 0;

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

    // QVariant 변환
    QVariant getVariantValue() const override
    {
        return QVariant::fromValue(m_value);
    }
    // QVariant -> T 변환
    void setVariantValue(const QVariant& val) override {
        if(val.isValid() && val.canConvert<T>()) {
            setValue(val.value<T>());
        }
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

template <typename Struct, typename MemberType>
class PropertyMemberAdapter : public PropertySignals
{
public:
    PropertyMemberAdapter(Property<Struct>& parentProperty, MemberType Struct::*memberPtr)
        : m_parentProperty(parentProperty), m_memberPtr(memberPtr)
    {

    }

    QVariant getVariantValue() const override {
        // 부모 Property 값에서 지정된 멤버의 값을 가져옴
        return QVariant::fromValue(m_parentProperty.value().*m_memberPtr);
    }

    void setVariantValue(const QVariant& val) override {
        if(val.isValid() && val.canConvert<MemberType>()) {
            // 부모 Property 값을 복사해와서
            Struct temp = m_parentProperty.value();
            // 지정된 멤버의 값을 수정한 뒤
            temp.*m_memberPtr = val.value<MemberType>();
            // 수정된 구조체로 부모 Property를 업데이트
            m_parentProperty.setValue(temp);
        }
    }

private:
    Property<Struct>& m_parentProperty;
    MemberType Struct::*m_memberPtr;
};

#endif // PROPERTY_H
