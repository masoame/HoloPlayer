#include "holomainwindow.h"
#include "./ui_holomainwindow.h"
#include "nettofiledialog.h"
#include"HoloTitleWidget.h"


#include "QMessageBox"
#include "qfiledialog.h"

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
    : QMainWindow(parent), drivefullwindows(this->ffmpeg_dirver)
    , ui(new Ui::HoloMainWindow)
{
    setWindowFlags(Qt::FramelessWindowHint /*| Qt::WindowSystemMenuHint*/ | Qt::WindowMinimizeButtonHint);
    ui->setupUi(this);
    QWidget* emTitle = new QWidget(this);
    ui->dockTitleWidget->setTitleBarWidget(emTitle);
    auto title = new HoloTitleWidget(nullptr);
    ui->dockTitleWidget->setWidget(title);
    connect(ui->openFile,SIGNAL(triggered()),this,SLOT(StartOpenFile()));
    connect(ui->netMode,SIGNAL(triggered()),this,SLOT(StartNetMode()));
    timer_id = this->startTimer(250);
}

HoloMainWindow::~HoloMainWindow()
{
    delete ui;
}

void HoloMainWindow::on_stop_play_clicked()
{
    if (this->drivefullwindows.target==nullptr || this->drivefullwindows.target->ThrPlay.joinable() == false) return;
    if (this->drivefullwindows.target->local_thread & SDLLayer::playing_thread)
    {
        SDL_PauseAudioDevice(this->drivefullwindows.device_id, 1);
        this->drivefullwindows.target->stop(SDLLayer::playing_thread);
    }
    else if(this->drivefullwindows.target->ThrPlay.joinable())
    {
        SDL_PauseAudioDevice(this->drivefullwindows.device_id, 0);
        this->drivefullwindows.target->run(SDLLayer::playing_thread);
    }
    else 
        this->drivefullwindows.StartPlayer();
}

void HoloMainWindow::timerEvent(QTimerEvent * event)
{
    if(this->drivefullwindows.target==nullptr) return;

    auto& audio_ptr = this->drivefullwindows.target->avframe_work[AVMEDIA_TYPE_AUDIO].first;
    if(audio_ptr==nullptr)return;
    if(!ui->time_slider->isSliderDown())
    {
        int sec=audio_ptr->pts * this->drivefullwindows.target->secBaseTime[AVMEDIA_TYPE_AUDIO];
        ui->timestamp->setText(QString::asprintf("%02d:%02d", sec / 60, sec % 60));
        ui->time_slider->setValue(sec);
    }
    if(this->drivefullwindows.target->local_thread & FFmpegLayer::playing_thread)
        ui->stop_play->setStyleSheet(str_StyleSheet_run);
    else
        ui->stop_play->setStyleSheet(str_StyleSheet_stop);
}

bool temp_isrun = false;
void HoloMainWindow::on_time_slider_sliderReleased()
{
    if(this->drivefullwindows.target==nullptr) return;

    temp_isrun = this->drivefullwindows.target->local_thread & FFmpegLayer::playing_thread;
    SDL_PauseAudioDevice(this->drivefullwindows.device_id, 1);
    this->drivefullwindows.target->stop(FFmpegLayer::playing_thread);
    this->drivefullwindows.target->seek_time(ui->time_slider->value());

    if(!this->drivefullwindows.target->ThrPlay.joinable()) this->drivefullwindows.StartPlayer();
    if(temp_isrun)
    on_stop_play_clicked();
}

void HoloMainWindow::on_time_slider_sliderMoved(int position)
{
    if(this->drivefullwindows.target==nullptr) return;
    ui->timestamp->setText(QString::asprintf("%02d:%02d", position/60,position%60));
}

void HoloMainWindow::StartOpenFile()
{
    QString filepath = QFileDialog::getOpenFileName(this, tr("打开文件"), "./", tr("video files(*.mp4 *.mkv *.flv);;All files(*.*)"));
    if (filepath.isEmpty() || filepath == "") return;
    if (ffmpeg_dirver.open(filepath.toStdString().c_str()) != FFmpegLayer::SUCCESS) return;
    this->drivefullwindows.InitPlayer("show_windows");
    this->drivefullwindows.StartPlayer();
    int sec = this->drivefullwindows.target->avfctx_input->duration / AV_TIME_BASE;
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
void HoloMainWindow::on_volume_slider_valueChanged(int value)
{
    this->drivefullwindows.volume = value;
}

