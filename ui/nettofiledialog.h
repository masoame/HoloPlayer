#ifndef NETTOFILEDIALOG_H
#define NETTOFILEDIALOG_H
#include"SpiderVideo.h"
#include <QDialog>

namespace Ui {
class NetToFileDialog;
}

class NetToFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NetToFileDialog(QWidget *parent = nullptr);
    ~NetToFileDialog();

private:
    Ui::NetToFileDialog *ui;
    SpiderVideo::SpiderTool _userdata;

private slots:
    void on_checkUrlBtn_clicked();
    void on_saveFileBtn_clicked();
};

#endif // NETTOFILEDIALOG_H
