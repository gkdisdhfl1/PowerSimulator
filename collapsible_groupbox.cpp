#include "collapsible_groupbox.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget *parent)
    : QWidget{parent}
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);

    toggleButton = new QToolButton;
    toggleButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toggleButton->setArrowType(Qt::DownArrow);
    toggleButton->setCheckable(true);
    toggleButton->setChecked(false);

    QLabel *titleLabel = new QLabel(title);

    headerLayout->addWidget(toggleButton);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    contentArea = new QWidget;
    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    contentArea->setVisible(false);
    contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(20, 5, 5, 5);

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(contentArea);

    connect(toggleButton, &QToolButton::toggled, this, [&](bool checked) {
        toggleButton->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
        contentArea->setVisible(checked);
    });
}

QVBoxLayout* CollapsibleGroupBox::contentLayoutPtr() {
    return contentLayout;
}
