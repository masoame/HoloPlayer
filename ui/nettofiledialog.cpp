#include "nettofiledialog.h"
#include "ui_nettofiledialog.h"
#include<qmessagebox.h>
#include"ParseJson.h"
NetToFileDialog::NetToFileDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NetToFileDialog)
{
    ui->setupUi(this);
}

NetToFileDialog::~NetToFileDialog()
{
    delete ui;
}

void NetToFileDialog::on_checkUrlBtn_clicked()
{
    if (ui->urlEdit->text().isEmpty() || ui->urlEdit->text() == "")return;

    QMessageBox::information(this, "测试", "测试");
    CURLcode code;
    auto str = ui->urlEdit->text().toStdString();
    code = this->_userdata.RequestInit(str.c_str());
    if (code != CURLE_OK) {
        QMessageBox::information(this, "error", "");
        return;
    }

    code = this->_userdata.SetOption(SpiderVideo::saveBuffer);
    if (code != CURLE_OK) {
        QMessageBox::information(this, "error", "");
        return;
    }
    code = this->_userdata.RequestSoure();
    if (code != CURLE_OK) {
        QMessageBox::information(this, "error", "");
        return;
    }
    auto results = ParseJson::ParseBilibili(_userdata.buffer.data());
    if (results.has_value()==false) {
        QMessageBox::information(this, "error", "");
        return;
    }
    QMessageBox::information(this, "success", "");
    for (auto& str : results.value())
    {
        qDebug() << str << "\n";
    }


    return;
}