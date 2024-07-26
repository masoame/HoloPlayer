#include "holomainwindow.h"
#include "./ui_holomainwindow.h"
#include "nettofiledialog.h"


#include "QMessageBox"
#include "qfiledialog.h"

using namespace BaseSDL;

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
    : QMainWindow(parent)
    , ui(new Ui::HoloMainWindow)
{
    bindPlayTool(this->ffmpeg_dirver);
    ui->setupUi(this);
    connect(ui->openFile,SIGNAL(triggered()),this,SLOT(StartOpenFile()));
    connect(ui->netMode,SIGNAL(triggered()),this,SLOT(StartNetMode()));
    timer_id = this->startTimer(250);
}

HoloMainWindow::~HoloMainWindow()
{
    BaseSDL::Destroy();
    delete ui;
}

void HoloMainWindow::on_stop_play_clicked()
{
    if (target == nullptr || target->ThrPlay.joinable() == false) return;
    if (target->local_thread & playing_thread)
    {
        SDL_PauseAudio(1);
        target->stop(playing_thread);
    }
    else if(target->ThrPlay.joinable())
    {
        SDL_PauseAudio(0);
        target->run(playing_thread);
    }
    else BaseSDL::StartPlayer();
}

void HoloMainWindow::timerEvent(QTimerEvent * event)
{
    if(BaseSDL::target==nullptr) return;

    auto& audio_ptr = BaseSDL::target->avframe_work[AVMEDIA_TYPE_AUDIO].first;
    if(audio_ptr==nullptr)return;
    if(!ui->time_slider->isSliderDown())
    {
        int sec=audio_ptr->pts * BaseSDL::target->secBaseTime[AVMEDIA_TYPE_AUDIO];
        ui->timestamp->setText(QString::asprintf("%02d:%02d", sec/60,sec%60));
        ui->time_slider->setValue(sec);
    }
    if(target->local_thread & playing_thread)
        ui->stop_play->setStyleSheet(str_StyleSheet_run);
    else
        ui->stop_play->setStyleSheet(str_StyleSheet_stop);
}

bool temp_isrun = false;
void HoloMainWindow::on_time_slider_sliderReleased()
{
    if(target==nullptr) return;

    temp_isrun = target->local_thread & playing_thread;
    SDL_PauseAudio(1);
    target->stop(playing_thread);
    target->seek_time(ui->time_slider->value());

    if(!target->ThrPlay.joinable()) StartPlayer();
    if(temp_isrun)
    on_stop_play_clicked();
}

void HoloMainWindow::on_time_slider_sliderMoved(int position)
{
    if(target==nullptr) return;
    ui->timestamp->setText(QString::asprintf("%02d:%02d", position/60,position%60));
}

void HoloMainWindow::StartOpenFile()
{
    QString filepath = QFileDialog::getOpenFileName(this,tr("打开文件"),"./",tr("video files(*.mp4 *.mkv *.flv);;All files(*.*)"));
    if(filepath.isEmpty() || filepath=="")return;
    if (ffmpeg_dirver.open(filepath.toStdString().c_str()) != BaseFFmpeg::SUCCESS) return;
    InitPlayer("show_windows");
    StartPlayer();
    int sec=target->avfctx_input->duration/AV_TIME_BASE;
    ui->total_time->setText(QString::number(sec/60)+":"+QString::number(sec%60));
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
    BaseSDL::Global_AudioRunning::volume=value;
}

