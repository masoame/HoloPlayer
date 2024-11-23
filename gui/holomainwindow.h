#ifndef HOLOMAINWINDOW_H
#define HOLOMAINWINDOW_H
#include "BaseSDL.h"
#include <qmainwindow.h>

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

    void on_time_slider_sliderReleased();

    void on_time_slider_sliderMoved(int position);

    void on_volume_slider_valueChanged(int value);

    void StartOpenFile();

    void StartNetMode();

private:

    void closeWindow();

    Ui::HoloMainWindow *ui;
    SDLLayer::DriveWindow drivewindows;
    int timer_id;
protected:
    void timerEvent(QTimerEvent * event) override;
    void resizeEvent(QResizeEvent *event) override;
};
#endif // HOLOMAINWINDOW_H
