#include "nettofiledialog.h"
#include "ui_nettofiledialog.h"
#include<qmessagebox.h>
#include<qfiledialog.h>
#include"ParseJson.h"
#include<qcheckbox.h>
#include<qoverload.h>

NetToFileDialog::NetToFileDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NetToFileDialog)
{
    ui->setupUi(this);
    connect(ui->listWidget, &QListWidget::itemSelectionChanged, [this]() {
        auto item = ui->listWidget->selectedItems();
        auto _checkbox = qobject_cast<QCheckBox*>(ui->listWidget->itemWidget(item[0]));

        qDebug() << "item changed" << _checkbox->text() << _checkbox->isChecked();

        auto str = _checkbox->text().toStdString();
        auto _data_ptr = _video_info.find(str);

        auto& _data = _data_ptr->second.all_data;

        ui->cb_quality->clear();
        for (auto& accept_description : ::ParseJson::BiliBili::GetSuportedQualities(_data)) {
            ui->cb_quality->addItem(QString::fromStdString(accept_description));
        }
        
        ui->cb_audio_format->clear();
        for (auto& audio_format : ::ParseJson::BiliBili::GetSuportedAudioCodecs(_data)) {
            ui->cb_audio_format->addItem(QString::fromStdString(audio_format));
        }
        
    });

    connect(ui->cb_quality, &QComboBox::currentTextChanged, [this](const QString& text) {

        auto item = ui->listWidget->selectedItems()[0];
        auto _checkbox = qobject_cast<QCheckBox*>(ui->listWidget->itemWidget(item));
        auto _title = _checkbox->text().toStdString();
        auto& _data = _video_info[_title].all_data;

        _video_info[_title].select_quality = text.toStdString();

        ui->cb_video_format->clear();
        for (auto& video_format : ::ParseJson::BiliBili::GetSuportedVideoCodecs(_data,text.toStdString())){
           ui->cb_video_format->addItem(QString::fromStdString(video_format));
        }
    });

    connect(ui->cb_audio_format, &QComboBox::currentTextChanged, [this](const QString& text) {
        auto item = ui->listWidget->selectedItems()[0];
        auto _checkbox = qobject_cast<QCheckBox*>(ui->listWidget->itemWidget(item));
        auto _title = _checkbox->text().toStdString();
        _video_info[_title].select_audio_codecs = text.toStdString();
    });

    connect(ui->cb_video_format, &QComboBox::currentTextChanged, [this](const QString& text) {
        auto item = ui->listWidget->selectedItems()[0];
        auto _checkbox = qobject_cast<QCheckBox*>(ui->listWidget->itemWidget(item));
        auto _title = _checkbox->text().toStdString();
        _video_info[_title].select_video_codecs = text.toStdString();
    });
}

NetToFileDialog::~NetToFileDialog()
{
    delete ui;
}

void NetToFileDialog::flush_combobox_data()
{
    
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
            
            auto[video_url, audio_url] = ::ParseJson::BiliBili::GetDownloadUrl(_video_info[_item->text().toStdString()]);
            qDebug() << _item->text() << "\n";
            qDebug() << "video_url:" << video_url << "\naudio_url:" << audio_url << "\n";

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

    code = this->_userdata.RequestInit(ui->urlEdit->text().toStdString());
    if (code != CURLE_OK) return;

    code = this->_userdata.SetOption(SpiderVideo::saveBuffer);
    if (code != CURLE_OK) return;

    code = this->_userdata.RequestSoure();
    if (code != CURLE_OK) return;

    auto results = ::ParseJson::BiliBili::ParseHTML(_userdata.buffer.data());
    if (results.has_value() == false) return;

    _video_info[results.value().first] 
        = ::ParseJson::BiliBili::Data{ 
        results.value().second, 
            {},
            {},
            {},
    };


    auto& _title = results.value().first;

    auto _item = new QListWidgetItem();
    auto _checkbox = new QCheckBox();
    ui->listWidget->addItem(_item);
    ui->listWidget->setItemWidget(_item, _checkbox);
    _checkbox->setText(QString::fromStdString(_title));
    _item->setSizeHint(_checkbox->sizeHint());

    connect(_checkbox, &QCheckBox::clicked, [this, _item]() {
        emit ui->listWidget->itemPressed(_item); // 自行发出 itemPressed 信号
        });
    _userdata.buffer.clear();
}