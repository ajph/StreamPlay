#include "config.h"
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QDebug>
#include <QDir>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>

PreferencesDialog::PreferencesDialog(Preferences *prefs, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    prefs(prefs)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);

    // General
    bool s;
    s = prefs->saveTorrentVideos();
    ui->chkSaveTorrentFiles->setChecked(s);
    ui->frmDownloadPath->setDisabled(!s);
    ui->leDownloadDirectory->setText(prefs->savePath());
    connect(ui->chkSaveTorrentFiles, SIGNAL(toggled(bool)), this, SLOT(setSaveTorrents(bool)));
    connect(ui->leDownloadDirectory, SIGNAL(editingFinished()), this, SLOT(setSavePath()));
    connect(ui->tbFindDir, SIGNAL(clicked()), this, SLOT(tbFindDir_click()));

    // BitTorrent
    ui->spListenPort->setValue(prefs->bittorrentPort());
    connect(ui->spListenPort, SIGNAL(valueChanged(int)), this, SLOT(setListenPort(int)));

    // License
    connect(ui->pbPurchase, SIGNAL(clicked()), this, SLOT(pbPurchase_click()));
    connect(ui->pbImportLicense, SIGNAL(clicked()), this, SLOT(pbImportLicense_click()));
}

void PreferencesDialog::setSavePath()
{
    prefs->setSavePath(ui->leDownloadDirectory->text());
}

void PreferencesDialog::setSaveTorrents(bool state)
{
    ui->frmDownloadPath->setDisabled(!state);
    prefs->setSaveTorrentVideos(state);

    // set save path to temp
    if (!state)
    {
        QString p = QDir::tempPath() + "/streamplay";
        prefs->setSavePath(p);
        ui->leDownloadDirectory->setText(p);
    }
}

void PreferencesDialog::setListenPort(int i)
{
    prefs->setBittorrentPort(i);
}

void PreferencesDialog::pbPurchase_click()
{
    QDesktopServices::openUrl(QUrl(CONFIG_PURCHASE_URL));
}

void PreferencesDialog::tbFindDir_click()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                    QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (dir.length() > 0)
    {
        ui->leDownloadDirectory->setText(dir);
        setSavePath();
    }
}

void PreferencesDialog::pbImportLicense_click()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import License", QDir::homePath(),
                                                     "License (*.streamplaylic)");
    if (fileName.length() > 0)
        emit importLicense(fileName);
}

void PreferencesDialog::closeEvent(QCloseEvent *event)
{
    // ensure save path is tmp if not saving torrents
    if (!prefs->saveTorrentVideos())
        setSaveTorrents(false);

    event->accept();
}

void PreferencesDialog::showEvent(QShowEvent *event)
{
    setRegState();
    event->accept();
}

void PreferencesDialog::setRegState()
{
    prefs->ch3();
    if (prefs->ch())
    {
        ui->lblRegistration->setText("Registered to "+prefs->regName()+"\n"
                                     "("+prefs->regEmail()+")\n"
                                     "Order "+prefs->regOrder());
        ui->stackLicense->setCurrentIndex(1); // show registered
    }
    else
    {
        ui->stackLicense->setCurrentIndex(0); // show unlicensed
    }
}

void PreferencesDialog::showLicenseTab()
{
    ui->tabWidget->setCurrentIndex(2);
    ui->tabWidget->setTabEnabled(2, true);
}

void PreferencesDialog::enableTabs()
{
    for (int i = 0; i < ui->tabWidget->count(); i++)
        ui->tabWidget->setTabEnabled(i, true);
}

void PreferencesDialog::disableTabs()
{
    for (int i = 0; i < ui->tabWidget->count(); i++)
        ui->tabWidget->setTabEnabled(i, false);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}
