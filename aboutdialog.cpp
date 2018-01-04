#include "config.h"
#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
    setWindowTitle("");

    ui->lblVersion->setText("v"+QString::number(CONFIG_VERSION_MAJ)+"."+QString::number(CONFIG_VERSION_MIN)+"."+QString::number(CONFIG_VERSION_REV));
    ui->textAcknowledgements->setSource(QUrl("qrc:/AboutDialog/other/acknowledgements.html"));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
