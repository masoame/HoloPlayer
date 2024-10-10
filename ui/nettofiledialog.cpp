#include "nettofiledialog.h"
#include "ui_nettofiledialog.h"
#include<qmessagebox.h>
#include<qfiledialog.h>
#include"ParseJson.h"
#include<qcheckbox.h>

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

void NetToFileDialog::on_saveFileBtn_clicked()
{
    QString filepath = QFileDialog::getExistingDirectory(this, tr("请选择保存地址"), tr("./"));
    if (filepath.isEmpty() || filepath == "") return;
    this->ui->pathEdit->setText(filepath);
}

void NetToFileDialog::on_btn_download_clicked()
{
    const auto num = ui->listWidget->count();
    for (int i = 0; i != num; i++)
    {
        auto _item = qobject_cast<QCheckBox*>(ui->listWidget->itemWidget(ui->listWidget->item(i)));
        if (_item != nullptr && _item->isChecked()) {

        }
    }

}

void NetToFileDialog::on_checkUrlBtn_clicked()
{
    if (ui->urlEdit->text().isEmpty() || ui->urlEdit->text() == "")return;

    CURLcode code = CURLE_OK;
    std::unique_ptr<CURLcode, decltype([](void* _code)->void
        {
            auto& __code = *static_cast<CURLcode*>(_code);
            if (__code != CURLE_OK) 
                QMessageBox::information(nullptr, "错误提示", curl_easy_strerror(__code));
            else
                QMessageBox::information(nullptr, "提示", "解析成功");
        }) > end(&code);

    //auto str = ui->urlEdit->text().toStdString();
    code = this->_userdata.RequestInit(ui->urlEdit->text().toStdString().c_str());
    if (code != CURLE_OK) return;

    code = this->_userdata.SetOption(SpiderVideo::saveBuffer);
    if (code != CURLE_OK) return;

    code = this->_userdata.RequestSoure();
    if (code != CURLE_OK) return;

    auto results = ParseJson::BiliBili::ParseHTML(_userdata.buffer.data());
    if (results.has_value() == false) return;

    auto supported_format = ParseJson::BiliBili::GetSuportedFormat(results.value().second);
    if (results.has_value() == false) return;

    auto& _title = results.value().first;
    _video_info[_title] = std::move(supported_format.value());

    auto _item = new QListWidgetItem();
    auto _checkbox = new QCheckBox();
    ui->listWidget->addItem(_item);
    ui->listWidget->setItemWidget(_item, _checkbox);
    _checkbox->setText(QString::fromStdString(_title));

    _item->setSizeHint(_checkbox->sizeHint());
}