#include<QApplication>
#include "holomainwindow.h"

int main(int argc, char* args[])
{
    QApplication app(argc, args);
    HoloMainWindow w;
    w.show();
    return app.exec();
}
