#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "preferences.h"

#include <QDialog>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(Preferences *prefs, QWidget *parent = 0);
    ~PreferencesDialog();
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void enableTabs();
    void disableTabs();
    void showLicenseTab();
    void setRegState();

private:
    Ui::PreferencesDialog *ui;
    Preferences *prefs;

signals:
    void importLicense(QString);

private slots:
    void setSaveTorrents(bool state);
    void setSavePath();
    void setListenPort(int i);
    void pbPurchase_click();
    void tbFindDir_click();
    void pbImportLicense_click();
};

#endif // PREFERENCESDIALOG_H
