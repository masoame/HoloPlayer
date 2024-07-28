#ifndef NETTOFILEDIALOG_H
#define NETTOFILEDIALOG_H

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
};

#endif // NETTOFILEDIALOG_H
