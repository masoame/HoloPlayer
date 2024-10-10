#ifndef NETTOFILEDIALOG_H
#define NETTOFILEDIALOG_H
#include"SpiderVideo.h"
#include"ParseJson.h"
#include <QDialog>
#include <map>
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
    std::map<std::string, std::pair<ParseJson::json, std::set<std::string>>> _video_info;
    SpiderVideo::SpiderTool _userdata;

private slots:
    void on_checkUrlBtn_clicked();
    void on_saveFileBtn_clicked();
    void on_btn_download_clicked();
};

#endif // NETTOFILEDIALOG_H
