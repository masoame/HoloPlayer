#pragma once

#include <QWidget>
#include "ui_HoloTitleWidget.h"

class HoloTitleWidget : public QWidget
{
	Q_OBJECT

public:
	HoloTitleWidget(QWidget *parent = nullptr);
	~HoloTitleWidget();

	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void OnMaxBtnClicked();

private:
	Ui::HoloTitleWidgetClass ui;
	QPoint m_dragStartPosition;

signals:
		void signal_close_clicked();
};
