#ifndef DATA_ROW_WIDGET_H
#define DATA_ROW_WIDGET_H

#include <QWidget>

class QLabel;

class DataRowWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DataRowWidget(const QString& name, const QString& unit, bool hasLine = true, QWidget* parent = nullptr);

public slots:
    void setValue(double value);

private:
    QLabel* m_valueLabel;
};

#endif // DATA_ROW_WIDGET_H
