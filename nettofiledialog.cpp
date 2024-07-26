#include "nettofiledialog.h"
#include "ui_nettofiledialog.h"
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
