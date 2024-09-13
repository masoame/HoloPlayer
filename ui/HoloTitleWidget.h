#pragma once

#include <QWidget>
#include "ui_HoloTitleWidget.h"

class HoloTitleWidget : public QWidget
{
	Q_OBJECT

public:
	HoloTitleWidget(QWidget *parent = nullptr);
	~HoloTitleWidget();

private:
	Ui::HoloTitleWidgetClass ui;

signals:
		void signal_close_clicked();
public:
		void OnMaxBtnClicked();
};
