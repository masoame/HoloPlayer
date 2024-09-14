#include "HoloTitleWidget.h"
#include <QMainWindow>
#include <QMouseEvent>
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
    //connect();
}

HoloTitleWidget::~HoloTitleWidget()
{

}

void HoloTitleWidget::mousePressEvent(QMouseEvent* event) {
    if (auto _parent = this->parentWidget()->parentWidget(); event->buttons() == Qt::LeftButton) {
        m_dragStartPosition = event->globalPos() -_parent->frameGeometry().topLeft();
        event->accept();
    }
}

void HoloTitleWidget::mouseMoveEvent(QMouseEvent* event) {
    if (auto _parent = this->parentWidget()->parentWidget(); event->buttons() & Qt::LeftButton) {
        _parent->move(event->globalPos() - m_dragStartPosition);
        event->accept();
    }
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

