#include "holomainwindow.h"
#include "./ui_holomainwindow.h"

#include "QMessageBox"
#include "qfiledialog.h"

HoloMainWindow::HoloMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HoloMainWindow)
{
    ui->setupUi(this);
    connect(ui->open_action,SIGNAL(triggered()),this,SLOT(test()));

    this->startTimer(250);

}

HoloMainWindow::~HoloMainWindow()
{
    delete ui;
    BaseSDL::Destroy();
}

void HoloMainWindow::on_stop_play_clicked()
{
    if(BaseSDL::target == nullptr) return;

    BaseSDL::Global_VideoRunning::is_pause?BaseSDL::run():BaseSDL::stop();

    if(BaseSDL::Global_VideoRunning::is_pause)
    ui->stop_play->setStyleSheet("                                                      \
                                    QPushButton:!hover{                                 \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page1.png)    \
                                    }                                                   \
                                    QPushButton:hover {                                 \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page0.png);   \
                                    }                                                   \
                                    QPushButton:pressed {                               \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page1.png);   \
                                    }                                                   \
                                 ");
    else
    ui->stop_play->setStyleSheet("                                                      \
                                    QPushButton:!hover{                                 \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page3.png)    \
                                    }                                                   \
                                    QPushButton:hover {                                 \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page2.png);   \
                                    }                                                   \
                                    QPushButton:pressed {                               \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page3.png);   \
                                    }                                                   \
                                 ");
}

void HoloMainWindow::timerEvent(QTimerEvent * event)
{
    if(BaseSDL::target==nullptr) return;

    auto& audio_ptr = BaseSDL::target->avframe_work[AVMEDIA_TYPE_AUDIO].first;
    if(audio_ptr==nullptr)return;
    if(!ui->time_slider->isSliderDown())
    {
        int sec=audio_ptr->pts * BaseSDL::target->secBaseAudio;
        ui->timestamp->setText(QString::asprintf("%02d:%02d", sec/60,sec%60));
        ui->time_slider->setValue(sec);
    }
}



void HoloMainWindow::on_time_slider_sliderPressed()
{
    if(BaseSDL::target==nullptr) return;
    BaseSDL::stop();
}


void HoloMainWindow::on_time_slider_sliderReleased()
{
    if(BaseSDL::target==nullptr) return;
    BaseSDL::target->seek_time(ui->time_slider->value());
    BaseSDL::run();
}

void HoloMainWindow::on_time_slider_sliderMoved(int position)
{
    if(BaseSDL::target==nullptr) return;
    ui->timestamp->setText(QString::asprintf("%02d:%02d", position/60,position%60));
}

void HoloMainWindow::test()
{
    QString filepath = QFileDialog::getOpenFileName(this);
    if(filepath.isEmpty() || filepath=="")return;

    BaseSDL::stop();
    if (ffmpeg_dirver.open(filepath.toStdString().c_str()) != BaseFFmpeg::SUCCESS) return;
    BaseSDL::InitPlayer(ffmpeg_dirver, "show_windows");
    BaseSDL::StartPlayer();
    int sec=BaseSDL::target->avfctx_input->duration/AV_TIME_BASE;
    ui->total_time->setText(QString::number(sec/60)+":"+QString::number(sec%60));
    ui->time_slider->setSliderPosition(0);
    ui->time_slider->setMaximum(sec);
    ui->stop_play->setStyleSheet("                                                      \
                                    QPushButton:!hover{                                 \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page1.png)    \
                                    }                                                   \
                                    QPushButton:hover {                                 \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page0.png);   \
                                    }                                                   \
                                    QPushButton:pressed {                               \
                                        border:none;                                    \
                                        image:url(:/Image/icon/Image/icon/page1.png);   \
                                    }                                                   \
                                 ");
}

