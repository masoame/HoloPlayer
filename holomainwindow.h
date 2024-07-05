#ifndef HOLOMAINWINDOW_H
#define HOLOMAINWINDOW_H
#include <qmainwindow.h>

#include "BaseSDL.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class HoloMainWindow;
}
QT_END_NAMESPACE

class HoloMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    HoloMainWindow(QWidget *parent = nullptr);
    ~HoloMainWindow();

private slots:

    void on_stop_play_clicked();

    void on_time_slider_sliderPressed();

    void on_time_slider_sliderReleased();

    void on_time_slider_sliderMoved(int position);

    void openvideo();

    void on_volume_slider_valueChanged(int value);

private:
    Ui::HoloMainWindow *ui;
    BaseFFmpeg ffmpeg_dirver;
protected:
    void timerEvent(QTimerEvent * event) override;
};
#endif // HOLOMAINWINDOW_H
