#include "holomainwindow.h"
#include "./ui_holomainwindow.h"
#include "nettofiledialog.h"
#include"HoloTitleWidget.h"
#include "QMessageBox"
#include "qfiledialog.h"

#include <iostream>
#include <syncstream>

constexpr const char str_StyleSheet_stop[]="            \
    QPushButton:!hover{                                 \
        border:none;                                    \
        image:url(:/Image/icon/Image/icon/page1.png)    \
}                                                       \
    QPushButton:hover {                                 \
        border:none;                                    \
        image:url(:/Image/icon/Image/icon/page0.png);   \
}                                                       \
    QPushButton:pressed {                               \
        border:none;                                    \
        image:url(:/Image/icon/Image/icon/page1.png);   \
}                                                       \
";

constexpr const char str_StyleSheet_run[]="             \
    QPushButton:!hover{                                 \
        border:none;                                    \
        image:url(:/Image/icon/Image/icon/page3.png)    \
}                                                       \
    QPushButton:hover {                                 \
        border:none;                                    \
        image:url(:/Image/icon/Image/icon/page2.png);   \
}                                                       \
    QPushButton:pressed {                               \
        border:none;                                    \
        image:url(:/Image/icon/Image/icon/page3.png);   \
}                                                       \
";

HoloMainWindow::HoloMainWindow(QWidget *parent)
    : QMainWindow(parent), drivewindows(new FFmpegLayer::PlayTool())
    , ui(new Ui::HoloMainWindow)
{
    ui->setupUi(this);
    ui->openGLWidget->setUpdatesEnabled(false);
    ui->openGLWidget->setAttribute(Qt::WA_OpaquePaintEvent);

    ui->dockTitleWidget->setTitleBarWidget(new QWidget(ui->dockTitleWidget));
    ui->dockTitleWidget->setWidget(new HoloTitleWidget(ui->dockTitleWidget));

    timer_id = this->startTimer(250);
    
    connect(ui->openFile, &QAction::triggered,this, &HoloMainWindow::StartOpenFile);
    connect(ui->netMode, &QAction::triggered,this, &HoloMainWindow::StartNetMode);
    
}

HoloMainWindow::~HoloMainWindow()
{
    delete ui;
}

void HoloMainWindow::on_stop_play_clicked()
{
    this->drivewindows.togglePause();
}

void HoloMainWindow::timerEvent([[maybe_unused]] QTimerEvent * event)
{
    if(this->drivewindows.play_tool ==nullptr) return;

    auto& audio_ptr = this->drivewindows.play_tool->avframe_work[AVMEDIA_TYPE_AUDIO].first;
    if(audio_ptr==nullptr)return;
    if(ui->time_slider->isSliderDown() == false){
        int sec=audio_ptr->pts * this->drivewindows.play_tool->secBaseTime[AVMEDIA_TYPE_AUDIO];
        ui->timestamp->setText(QString::asprintf("%02d:%02d", sec / 60, sec % 60));
        ui->time_slider->setValue(sec);
    }
    if(this->drivewindows.is_pause == false)
        ui->stop_play->setStyleSheet(str_StyleSheet_run);
    else
        ui->stop_play->setStyleSheet(str_StyleSheet_stop);
}

void HoloMainWindow::resizeEvent([[maybe_unused]] QResizeEvent* event)
{
    this->drivewindows.ReSize(this->ui->openGLWidget->width(), this->ui->openGLWidget->height());
}


void HoloMainWindow::on_time_slider_sliderReleased()
{
    this->drivewindows.play_tool->seek_time(ui->time_slider->value());
}

void HoloMainWindow::on_time_slider_sliderMoved(int position)
{
    if(this->drivewindows.play_tool ==nullptr) return;
    ui->timestamp->setText(QString::asprintf("%02d:%02d", position/60,position%60));
}

void HoloMainWindow::StartOpenFile()
{
    QString filepath = QFileDialog::getOpenFileName(this, tr("打开文件"), "./", tr("video files(*.mp4 *.mkv *.flv);;All files(*.*)"));
    if (filepath.isEmpty() || filepath == "") return;

    if (this->drivewindows.play_tool->open(filepath.toStdString().c_str()) != FFmpegLayer::SUCCESS) return;

    this->drivewindows.InitPlayer(this->ui->openGLWidget->width(), this->ui->openGLWidget->height(),reinterpret_cast<void*>(this->ui->openGLWidget->winId()));
    this->drivewindows.StartPlayer();
    int sec = this->drivewindows.play_tool->avfctx_input->duration / AV_TIME_BASE;
    ui->total_time->setText(QString::number(sec / 60) + ":" + QString::number(sec % 60));
    ui->time_slider->setSliderPosition(0);
    ui->time_slider->setMaximum(sec);
}

void HoloMainWindow::StartNetMode()
{
    auto a = new NetToFileDialog(this);
    a->setAttribute(Qt::WA_DeleteOnClose);
    a->exec();
}
void HoloMainWindow::closeWindow()
{
    qDebug() << "closeWindow";
    this->close();

}
void HoloMainWindow::on_volume_slider_valueChanged(int value)
{
    this->drivewindows.volume = value;
}

