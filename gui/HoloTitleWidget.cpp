#include "HoloTitleWidget.h"
#include <QMainWindow>
#include <QMouseEvent>
#include <QMenu>
#include <qmenubar.h>
#include<qtoolbutton.h>

const QString format = "QToolButton::menu-indicator{width:0px;}QToolButton{ margin:-3px;}";

HoloTitleWidget::HoloTitleWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    auto _parent = parent->parentWidget();
    _parent->setWindowFlags(Qt::FramelessWindowHint /*| Qt::WindowSystemMenuHint*/ | Qt::WindowMinimizeButtonHint);

	connect(ui.closeButton, &QPushButton::clicked, _parent, &QWidget::close);
	connect(ui.maximizeButton, &QPushButton::clicked, this, &HoloTitleWidget::OnMaxBtnClicked);
    connect(ui.minimizeButton, &QPushButton::clicked, _parent, &QWidget::showMinimized);
    
    auto menuBar = qobject_cast<QMainWindow*>(_parent)->menuBar();
    for (auto& obj : menuBar->children())
    {
        auto menu = qobject_cast<QMenu*>(obj);
        if (menu == nullptr)continue;

        auto btn = new QToolButton(this);
        btn->setText(menu->title());
        btn->setMinimumSize(QSize(55, 25));

        btn->setMenu(menu);
        btn->setPopupMode(QToolButton::InstantPopup);
        ui.menuLayout->addWidget(btn);
        btn->setStyleSheet(format);
    }
    menuBar->hide();

}

HoloTitleWidget::~HoloTitleWidget()
{

}

void HoloTitleWidget::mousePressEvent(QMouseEvent* event) {
    if (auto _parent = this->parentWidget()->parentWidget(); event->buttons() == Qt::LeftButton) {
        m_dragStartPosition = event->globalPosition() -_parent->frameGeometry().topLeft();
        event->accept();
    }
}

void HoloTitleWidget::mouseMoveEvent(QMouseEvent* event) {
    if (auto _parent = this->parentWidget()->parentWidget(); event->buttons() & Qt::LeftButton) {
        _parent->move((event->globalPosition() - m_dragStartPosition).toPoint());
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

