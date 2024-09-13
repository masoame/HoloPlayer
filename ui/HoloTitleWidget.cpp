#include "HoloTitleWidget.h"
#include <QMainWindow>
HoloTitleWidget::HoloTitleWidget(QWidget *parent)
	: QWidget(parent)
{
    Q_ASSERT(parent != nullptr);

	ui.setupUi(this);

    auto _parent = parent->parentWidget();
    _parent->setWindowFlags(Qt::FramelessWindowHint /*| Qt::WindowSystemMenuHint*/ | Qt::WindowMinimizeButtonHint);
	connect(ui.closeButton, &QPushButton::clicked, _parent, &QWidget::close);
	connect(ui.maximizeButton, &QPushButton::clicked, this, &HoloTitleWidget::OnMaxBtnClicked);
    connect(ui.minimizeButton, &QPushButton::clicked, _parent, &QWidget::showMinimized);
}

HoloTitleWidget::~HoloTitleWidget()
{
}

void HoloTitleWidget::OnMaxBtnClicked()
{
    auto parent = this->parentWidget()->parentWidget();
    if (parent->isMaximized() == true){
        parent->showNormal();
    }
    else{
        parent->showMaximized();
    }
}

