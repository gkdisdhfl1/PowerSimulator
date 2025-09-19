#ifndef COLLAPSIBLE_GROUPBOX_H
#define COLLAPSIBLE_GROUPBOX_H

#include <QWidget>

class QToolButton;
class QLabel;
class QVBoxLayout;

class CollapsibleGroupBox : public QWidget
{
    Q_OBJECT
public:
    explicit CollapsibleGroupBox(const QString& title, QWidget *parent = nullptr);
    QVBoxLayout* contentLayoutPtr();

private:
    QToolButton *toggleButton;
    QWidget *contentArea;
    QVBoxLayout *contentLayout;
};

#endif // COLLAPSIBLE_GROUPBOX_H
