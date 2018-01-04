#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "aboutdialog.h"
#include "preferencesdialog.h"
#include "preferences.h"
#include "airplay.h"
#include "torrentstream.h"

#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    #include "cocoaInitializer.h"
    #include "sparkleautoupdater.h"
#endif
#endif

#include <QMainWindow>
#include <QActionGroup>
#include <QMovie>
#include <QSettings>
#include <QElapsedTimer>
#include <QUdpSocket>

class QHostInfo;
class QNetworkReply;

class BonjourServiceBrowser;
class BonjourServiceResolver;
class BonjourBrowseRecord;
class HttpVideo;
class HlsVideo;
class AirPlay;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void menuOpenFile();
    void menuClearRecentFiles();
    void menuLoadSubtitles();
    void menuCheckForUpdates();
    void menuPreferences();
    void menuAbout();
    void menuHelp();
    void menuReportAProblem();
    void updateRecentFiles();
    void loadLicense(QString fileName);
    void flushBuffer();

private:
    enum state {
        STATE_INIT,
        STATE_FINDING_DEVICES,
        STATE_WAITING_TORRENT,
        STATE_ADDING_TORRENT,
        STATE_GETTING_TORRENT_INFO,
        STATE_BUFFERING,
        STATE_PREPARING_STREAM,
        STATE_CONNECTING_DEVICE,
        STATE_STREAMING
    };

    enum chState {
        CH_STATE_WAITING_START,
        CH_STATE_CHECK_TYP,
        CH_STATE_WAIT_RIFF,
        CH_STATE_GET_NEXT_ATOM,
        CH_STATE_GET_MOOV,
        CH_STATE_DONE,
    };

    enum fileType {
        TYPE_URL,
        TYPE_TORRENT,
        TYPE_VIDEO
    };

    struct InputFile {
        QString path;
        QString storagePath;
        fileType type;        
    };

    struct Atom {
        int index;
        quint64 pos;
        quint64 size;
        quint32 name;
    };

#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    AutoUpdater* updater;
    CocoaInitializer initializer;
#endif
#endif

    Ui::MainWindow *ui;
    state appState, appStatePrev;
    QMovie spinner;
    QTimer timerStateLoop;
    TorrentStream *torrentStream;
    HttpVideo *httpVideo;
    HlsVideo *hlsVideo;
    AirPlay *airplay;
    BonjourServiceBrowser *atvBrowser;
    BonjourServiceBrowser *gcBrowser;
    BonjourServiceResolver *bonjourResolver;
    Preferences *prefs;
    InputFile fileInput;
    chState checkHeaderState;
    Atom chAtom;
    QByteArray header;
    int currentPiece;
    QVector<Atom> atoms;
    bool bufferFlushed;
    QMenu *menuDevices;
    QActionGroup *devicesActGrp;
    bool loadingSubtitles;
    void setState(state s);
    void loadInput();
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *de);
    void addRecentFile(QString name, QString path);    
    bool checkHeader();    
    QString srtFile;
    PreferencesDialog *prefDialog;
    AboutDialog aboutDialog;
    QElapsedTimer streamingElapsed;
    QUdpSocket udp;


private slots:
    void updateDevices(const QList<BonjourBrowseRecord> &list);
    void deviceResolved(const QHostInfo &hostInfo, int, QString txtRecord);
    void stateLoop();
    void openRecentFile();
    void deviceClicked();
};

#endif // MAINWINDOW_H
