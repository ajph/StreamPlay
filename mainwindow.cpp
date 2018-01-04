#include "config.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "bonjourservicebrowser.h"
#include "bonjourserviceresolver.h"
#include "torrentstream.h"
#include "hlsvideo.h"
#include "airplay.h"
#include "moovfaststart.h"

#include <QMainWindow>
#include <QHostInfo>
#include <QTimer>
#include <QTcpSocket>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QWidgetAction>
#include <QtCore/qmath.h>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    torrentStream(), httpVideo(), hlsVideo(), airplay(), bonjourResolver(), header(), devicesActGrp()
{
    ui->setupUi(this);
    setAcceptDrops(true);

    // init
    loadingSubtitles = false;
    srtFile = "";

    // setup ui elements
    spinner.setFileName(":/MainWindow/images/spinner.gif");
    spinner.start();

    menuDevices = new QMenu(ui->toolButton);
    ui->toolButton->setPopupMode(QToolButton::InstantPopup);
    ui->toolButton->setMenu(menuDevices);

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(10000);
    ui->progressBar->setValue(0);

    setState(STATE_INIT);
    stateLoop();

    // load prefs
    prefs = new Preferences(this);
    connect(prefs, SIGNAL(recentFilesUpdated()), this, SLOT(updateRecentFiles()));
    prefs->recentlyOpenedFiles(); // trigger signal
    prefDialog = new PreferencesDialog(prefs,this);
    connect(prefDialog, SIGNAL(importLicense(QString)), this, SLOT(loadLicense(QString)));

    if (prefs->ch())
        setWindowTitle("StreamPlay");

    // check for updates
#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    updater = new SparkleAutoUpdater(CONFIG_UPDATE_URL);
    updater->checkForUpdates();
#endif
#endif

    // clear stream dir
    QDir(prefs->streamPath()).removeRecursively();

    // ensure directories exist
    QDir().mkpath(prefs->savePath());
    QDir().mkpath(prefs->streamPath());

    qDebug() << "save path" << prefs->savePath();
    qDebug() << "stream path" << prefs->streamPath();

    // setup device browsers
    atvBrowser = new BonjourServiceBrowser(this);
    connect(atvBrowser, SIGNAL(currentBonjourBrowseRecordsChanged(const QList<BonjourBrowseRecord> &)),
        this, SLOT(updateDevices(const QList<BonjourBrowseRecord> &)));

    // start torrent client
    try {
        torrentStream = new TorrentStream(prefs->savePath(), prefs->bittorrentPort(), this);
    }
    catch (int e) {
        //mb("error starting torrent client");
        return;
    }

    // start app state machine loop
    connect(&timerStateLoop, SIGNAL(timeout()), this, SLOT(stateLoop()));
    timerStateLoop.start(1000);
}

void MainWindow::stateLoop()
{
    TorrentStream::Stats s;
    QString subText;
    int prg;
    switch (appState)
    {
    case STATE_INIT:
        ui->lblAction->setMovie(&spinner);
        ui->lblStatus->setText("Starting services");
        ui->lblSubStatus->setText("");
        if (timerStateLoop.isActive())
            setState(STATE_FINDING_DEVICES);
        break;

    case STATE_FINDING_DEVICES:
        ui->lblAction->setMovie(&spinner);
        ui->lblStatus->setText("Searching for devices");
        //ui->lblSubStatus->setText("Please wait...");
        if (menuDevices->actions().length() > 0)
            setState(STATE_WAITING_TORRENT);
        break;

    case STATE_WAITING_TORRENT:
        ui->lblAction->setPixmap(QPixmap(":/MainWindow/images/drop.png"));
        ui->lblStatus->setText("Drop Media Here");
        break;

    case STATE_ADDING_TORRENT:
        ui->lblAction->setMovie(&spinner);
        ui->lblStatus->setText("Adding Torrent");
        ui->lblSubStatus->setText(fileInput.path);
        if (torrentStream->getStatus() == TorrentStream::STATE_RETRIEVING_METADATA)
            setState(STATE_GETTING_TORRENT_INFO);
        break;

    case STATE_GETTING_TORRENT_INFO:
        ui->lblAction->setMovie(&spinner);
        ui->lblStatus->setText("Getting Torrent Info");
        ui->lblSubStatus->setText(fileInput.path);
        if (torrentStream->getStatus() == TorrentStream::STATE_BUFFERING)
            setState(STATE_BUFFERING);
        break;        

    case STATE_BUFFERING:
        ui->lblAction->setMovie(&spinner);
        ui->lblStatus->setText("Buffering");
        subText = s.name;
        if (fileInput.type == TYPE_VIDEO)
        {
            subText = QFileInfo(fileInput.path).fileName();
        }
        else
        {
            s = torrentStream->getStats();
            subText = s.name + "\n";
            subText.append(QString::number(s.dSpeed / 1000, 'g', 4) + "kbps");
        }
        ui->lblSubStatus->setText(subText);
        ui->progressBar->setValue(2500);
        if (checkHeader() && !loadingSubtitles)
            setState(STATE_PREPARING_STREAM);
        break;

    case STATE_PREPARING_STREAM:
        ui->lblAction->setMovie(&spinner);
        ui->lblStatus->setText("Buffering");
        if (fileInput.type != TYPE_VIDEO)
        {
            s = torrentStream->getStats();
            subText = s.name + "\n";
            subText.append(QString::number(s.dSpeed / 1000, 'g', 4) + "kbps");
            ui->lblSubStatus->setText(subText);
        }
        flushBuffer();
        prg = 2500 + ((qMax(hlsVideo->getCurrentSegment(), 0)+1)*2500);
        ui->progressBar->setValue(prg);
        if (hlsVideo && hlsVideo->isReadyForStreaming())
            setState(STATE_CONNECTING_DEVICE);

        break;

    case STATE_CONNECTING_DEVICE:
        if (!ui->lblAction->movie())
        {
            ui->lblAction->clear();
            ui->lblAction->setMovie(&spinner);
        }
        ui->lblStatus->setText("Connecting Device");        
        if (fileInput.type != TYPE_VIDEO)
        {
            s = torrentStream->getStats();
            subText = s.name;
            ui->lblSubStatus->setText(subText);
        }
        flushBuffer();
        ui->progressBar->setValue(0);
        if (airplay && airplay->getStatus() == AirPlay::CONNECTED)
            setState(STATE_STREAMING);
        break;

    case STATE_STREAMING:
        ui->toolButton->setIcon(QIcon(":/MainWindow/images/airplay-on.png"));
        ui->lblAction->setPixmap(QPixmap(":/MainWindow/images/stream.png"));
        ui->lblStatus->setText("Streaming");
        flushBuffer();
        prg = qCeil((float)hlsVideo->getCurrentSegment() / (float)(hlsVideo->getTotalSegments() - 1) * 10000.0);
        ui->progressBar->setValue(prg);
        if (streamingElapsed.hasExpired(900000) && !prefs->ch())
        {
            airplay->pause();
            prefDialog->disableTabs();
            prefDialog->showLicenseTab();
            if (!prefDialog->isVisible())
            {
                prefDialog->show();
                prefDialog->activateWindow();
                prefDialog->raise();
                prefDialog->setFocus();
            }
        }
        break;
    }
}

void MainWindow::setState(state s)
{
    switch (s)
    {
    case STATE_INIT:
        break;

    case STATE_FINDING_DEVICES:
        // browse for devices
        atvBrowser->browseForServiceType(QLatin1String("_airplay._tcp"));
        //gcBrowser->browseForServiceType(QLatin1String("_googlecast._tcp"));
        break;

    case STATE_WAITING_TORRENT:
        // do nothing
        break;

    case STATE_ADDING_TORRENT:
        // add torrent
        torrentStream->start(fileInput.path);
        break;

    case STATE_GETTING_TORRENT_INFO:
        // do nothing
        break;

    case STATE_BUFFERING:
        // reset states
        ui->progressBar->setValue(0);
        currentPiece = 0;
        bufferFlushed = false;
        header.clear();
        checkHeaderState = CH_STATE_WAITING_START;
        // record recent
        if (fileInput.type != TYPE_VIDEO) {
            fileInput.storagePath = torrentStream->getFilePath();
            prefs->addRecentlyOpenedFile(torrentStream->getStats().name, fileInput.path);
        }
        else {
            prefs->addRecentlyOpenedFile(QFileInfo(fileInput.storagePath).fileName(), fileInput.path);
        }
        break;

    case STATE_PREPARING_STREAM:
        // start hls transcoding server                
        hlsVideo = new HlsVideo(prefs->streamPath(), this);
        hlsVideo->start(fileInput.storagePath, srtFile, 8888);
        break;

    case STATE_CONNECTING_DEVICE:        
        // resolve & connect to airplay
        if (devicesActGrp->checkedAction() == 0) return;

        if (airplay) {
            delete airplay;
            airplay = NULL;
        }
        ui->toolButton->setIcon(QIcon(":/MainWindow/images/airplay-off.png"));

        // setup resolver
        if (bonjourResolver) delete bonjourResolver;
        bonjourResolver = new BonjourServiceResolver(0);
        connect(bonjourResolver, SIGNAL(bonjourRecordResolved(const QHostInfo &, int, const QString)),
                this, SLOT(deviceResolved(const QHostInfo &, int, const QString)));

        bonjourResolver->resolveBonjourRecord(devicesActGrp->checkedAction()->data().value<BonjourBrowseRecord>());
        break;

    case STATE_STREAMING:
        airplay->play("http://"+airplay->getLocalIp().toString()+":8888/a.m3u8");
        srtFile = ""; // clear loaded subtitles
        streamingElapsed.restart();
        break;
    }

    appStatePrev = appState;
    appState = s;
}

void MainWindow::updateRecentFiles()
{
    ui->menuOpen_Recent->clear();

    QVector<Preferences::RecentFile> *recentFiles = prefs->recentlyOpenedFiles();
    for (QVector<Preferences::RecentFile>::iterator it = recentFiles->begin(); it != recentFiles->end(); ++it)
    {
        QAction *action = new QAction(this);
        action->setText(it->name);
        action->setData(it->path);
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
        ui->menuOpen_Recent->addAction(action);
    }
    ui->menuOpen_Recent->setEnabled( (recentFiles->length() > 0) );

    // add clear recent button
    ui->menuOpen_Recent->addSeparator();
    QAction *clearAction = new QAction(this);
    clearAction->setText("Clear Recent");
    connect(clearAction, SIGNAL(triggered()), this, SLOT(menuClearRecentFiles()));
    ui->menuOpen_Recent->addAction(clearAction);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (appState != STATE_INIT && appState != STATE_FINDING_DEVICES)
        e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *de)
{
    QString dr = de->mimeData()->text().toLatin1().data();
    if (dr.startsWith("file://")) dr.remove(0, 7);
    fileInput.path = dr;
    loadInput();
}

void MainWindow::menuAbout()
{
    aboutDialog.show();
}

void MainWindow::menuHelp()
{
    QDesktopServices::openUrl(QUrl(CONFIG_HELP_URL));
}

void MainWindow::menuReportAProblem()
{
    QDesktopServices::openUrl(QUrl(CONFIG_PROBLEM_URL));
}

void MainWindow::menuLoadSubtitles()
{
    loadingSubtitles = true;
    srtFile = QFileDialog::getOpenFileName(this, "Load Subtitles", QDir::homePath(),
                                             "Subtitles (*.srt)");
    qDebug() << srtFile;
    loadingSubtitles = false;
}

void MainWindow::menuOpenFile()
{
    fileInput.path = QFileDialog::getOpenFileName(this, "Load Media", QDir::homePath(),
                                             "Videos (*.mov *.avi *.mp4 *.mkv *.m4a *.ts *.wmv);;Torrents (*.torrent)");
    if (fileInput.path.length() > 0)
        loadInput();
}

void MainWindow::openRecentFile()
{
    QAction *action = (QAction *)sender();
    fileInput.path = action->data().toString();
    loadInput();
}

void MainWindow::menuClearRecentFiles()
{
    prefs->clearRecentlyOpenedFiles();
}

void MainWindow::menuCheckForUpdates()
{
#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG
    updater->checkForUpdates();
#endif
#endif
}

void MainWindow::menuPreferences()
{
    prefDialog->setModal(false);
    prefDialog->show();
}


void MainWindow::loadInput()
{
    prefs->ch2();

    if (fileInput.path.length() == 0)
        return;

    // stop current stream
    if (airplay) {
        delete airplay;
        airplay = NULL;
    }
    if (hlsVideo) {
        delete hlsVideo;
        hlsVideo = NULL;
    }
    ui->toolButton->setIcon(QIcon(":/MainWindow/images/airplay-off.png"));
    ui->lblAction->clear();
    ui->lblAction->setMovie(&spinner);

    // set start state
    if (fileInput.path.endsWith(".torrent"))
    {
        fileInput.type = TYPE_TORRENT;
        setState(STATE_ADDING_TORRENT);
    }
    else if (fileInput.path.startsWith("http") || fileInput.path.startsWith("magnet"))
    {
        fileInput.type = TYPE_URL;
        setState(STATE_ADDING_TORRENT);
    }
    else
    {
        fileInput.type = TYPE_VIDEO;
        fileInput.storagePath = fileInput.path;
        setState(STATE_BUFFERING);
    }
}

void MainWindow::flushBuffer()
{
    if (!hlsVideo || !hlsVideo->isReadyForInput() || bufferFlushed) return;

    QFile f(fileInput.storagePath);
    if(!f.open(QIODevice::ReadOnly)) return;

    if (fileInput.type == TYPE_VIDEO)
    {
        quint64 startByte = 0;
        if (header.length() > 0)
        {
            hlsVideo->encode(&header);
            header.clear();
            startByte = atoms.first().size;
        }
        f.seek(startByte);
        QByteArray data = f.read(f.size() - startByte);
        hlsVideo->encode(&data);
        bufferFlushed = true;
    }
    else // torrent
    {
        if (header.length() > 0)
        {
            qDebug() << "writing header";
            hlsVideo->encode(&header);
            header.clear();

            // write remainder of the first piece
            f.seek(atoms.first().size);

            int sz = torrentStream->getPieceLength() - atoms.first().size;
            QByteArray data = f.read(sz);

            hlsVideo->encode(&data);
            currentPiece = 1;
        }

        int pieceOffset = torrentStream->getFileOffset() / torrentStream->getPieceLength();
        int startPiece = currentPiece;
        while (torrentStream->hasPiece(currentPiece + pieceOffset) && ((currentPiece - startPiece) < 10))
            currentPiece++;
        currentPiece--;
        if (currentPiece <= startPiece)
        {
            currentPiece = startPiece;
            return;
        }

        // seek to st
        quint64 st = ((quint64)startPiece * torrentStream->getPieceLength());
        if (st >= torrentStream->getFileSize())
        {
            qDebug() << "here 1" << startPiece << st << torrentStream->getPieceLength() << torrentStream->getFileSize();
            bufferFlushed = true;
            return;
        }
        f.seek(st);

        // read sz bytes
        quint64 sz = ((quint64)(currentPiece - startPiece) * torrentStream->getPieceLength());
        if (st + sz > torrentStream->getFileSize())
        {
            qDebug() << "here 2" << st+sz << torrentStream->getFileSize();
            sz = torrentStream->getFileSize() - st;
            bufferFlushed = true;
        }
        QByteArray data = f.read(sz);        

        qDebug() << "writing" << (currentPiece - startPiece) << "pieces." << "current piece" << currentPiece;
        //qDebug() << "start" << st << "size" << sz;

        // encode
        hlsVideo->encode(&data);
    }
    f.close();
}

bool MainWindow::checkHeader()
{
    switch (checkHeaderState)
    {
    case CH_STATE_WAITING_START:
        if (fileInput.type != TYPE_VIDEO
            && !torrentStream->hasBytes(0, 8))
                return false;

        atoms.clear();
        checkHeaderState = CH_STATE_CHECK_TYP;
        break;

    case CH_STATE_CHECK_TYP:
    {
        QFile f(fileInput.storagePath);
        if (!f.open(QIODevice::ReadOnly))
                return false;

        qDebug() << "checking header";

        // read first 8 bytes from file
        quint32 b1, b2;
        QDataStream ds(&f);
        ds >> b1;
        ds >> b2;

        // check for mp4
        if (b2 == 0x66747970) // "ftyp"
        {
            chAtom.index = 0;
            chAtom.pos = 0;
            chAtom.name = b2;
            if (b1 == 1)
                ds >> chAtom.size;
            else
                chAtom.size = b1;
            atoms.push_back(chAtom);
            checkHeaderState = CH_STATE_GET_NEXT_ATOM;
            qDebug() << "ftyp found";
        }
        else if (b1 == 0x52494646) // "RIFF"
        {
            checkHeaderState = CH_STATE_WAIT_RIFF;
            qDebug() << "RIFF found";
        }
        else
        {
            checkHeaderState = CH_STATE_DONE;
            qDebug() << "no special header found" << hex << b1 << b2;
        }

        f.close();
        break;
    }
    case CH_STATE_WAIT_RIFF:
    {
        // wait for first 10kb
        if (fileInput.type != TYPE_VIDEO
            && !torrentStream->hasBytes(0, 10000))
                return false;

        checkHeaderState = CH_STATE_DONE;
        break;
    }
    case CH_STATE_GET_NEXT_ATOM:
    {
        if (fileInput.type != TYPE_VIDEO
            && !torrentStream->hasBytes(chAtom.pos + chAtom.size, 16))
        {
            torrentStream->prioritizeBytes(chAtom.pos + chAtom.size, 16);
            return false;
        }

        QFile f(fileInput.storagePath);
        if (!f.open(QIODevice::ReadOnly))
                return false;

        quint64 fileSize = 0;
        if (fileInput.type != TYPE_VIDEO)
            fileSize = torrentStream->getFileSize() - chAtom.pos;
        else
            fileSize = f.size();

        // read from file
        quint32 sz, typ;
        QDataStream ds(&f);
        ds.device()->seek(chAtom.pos + chAtom.size);
        ds >> sz;
        ds >> typ;

        chAtom.index += 1;
        chAtom.pos += chAtom.size;
        chAtom.name = typ;
        if (sz == 1) // 64-bit (wide) atom
            ds >> chAtom.size;
        else if (sz == 0) // atom spans to end of file
            chAtom.size = fileSize - chAtom.pos;
        else // size ok
            chAtom.size = sz;

        atoms.push_back(chAtom);
        qDebug() << "found atom " << hex << chAtom.name << "at" << chAtom.pos << "sz" << chAtom.size;

        if (chAtom.name == 0x6D6F6F76) // moov
        {
            if (chAtom.index == 1)
                checkHeaderState = CH_STATE_DONE;
            else
                checkHeaderState = CH_STATE_GET_MOOV;
            qDebug() << "moov atom found at index " << chAtom.index;
        }
        else
        {
            if (sz == 0 || chAtom.pos + chAtom.size >= fileSize)
            {
                qDebug() << "Could not find moov atom!";
                checkHeaderState = CH_STATE_DONE;
            }
        }

        f.close();
        break;
    }
    case CH_STATE_GET_MOOV:
    {
        if (fileInput.type != TYPE_VIDEO
            && !torrentStream->hasBytes(chAtom.pos, chAtom.size))
        {
            qDebug() << "prioritizing bytes " << hex << chAtom.pos << chAtom.size;
            torrentStream->prioritizeBytes(chAtom.pos, chAtom.size);
            return false;
        }

        QFile f(fileInput.storagePath);
        if (!f.open(QIODevice::ReadOnly))
                return false;

        qDebug() << "retrieved moov atom";

        // read ftyp
        f.seek(atoms.first().pos);
        QByteArray ftyp = f.read(atoms.first().size);

        // read moov
        f.seek(chAtom.pos);
        QByteArray moov = f.read(chAtom.size);

        f.close();

        moovFastStart fs(&moov, this);
        fs.offsetStcos();

        // build header
        header.clear();
        header.append(ftyp);
        header.append(moov);

        checkHeaderState = CH_STATE_DONE;
        break;
    }
    case CH_STATE_DONE:
        return true;
    }

    return false;
}

void MainWindow::deviceResolved(const QHostInfo &hostInfo, int port, QString txtRecord)
{
    const QList<QHostAddress> &addresses = hostInfo.addresses();
    if (addresses.isEmpty())
        return;

    // send wake-on-lan
    int s = txtRecord.indexOf("deviceid=");
    if (s > -1)
    {
        QString mac = txtRecord.mid(s+9, 17);
        QByteArray macDst = mac.replace(":", "").toStdString().c_str();
        QByteArray magic = QByteArray::fromHex("ffffffffffff");
        for (int i = 0; i < 16; i++) magic.append(QByteArray::fromHex(macDst));
        qDebug() << "sending wol";
        udp.writeDatagram(magic.data(), magic.size(), QHostAddress::Broadcast, 7);
        udp.writeDatagram(magic.data(), magic.size(), QHostAddress::Broadcast, 9);
    }

    // connect
    airplay = new AirPlay(addresses.first().toString(), port, this);
}

void MainWindow::updateDevices(const QList<BonjourBrowseRecord> &list)
{
    BonjourBrowseRecord current;
    if (devicesActGrp && devicesActGrp->checkedAction() != 0)
        current = devicesActGrp->checkedAction()->data().value<BonjourBrowseRecord>();

    menuDevices->clear();
    devicesActGrp = new QActionGroup(menuDevices);
    foreach (BonjourBrowseRecord record, list) {
        QVariant variant;
        variant.setValue(record);
        QAction *action = new QAction(devicesActGrp);
        action->setText(record.serviceName);
        action->setData(variant);
        action->setCheckable(true);
        action->setChecked(current == record); // select current
        connect(action, SIGNAL(triggered()), this, SLOT(deviceClicked()));
        menuDevices->addAction(action);
    }

    // select first
    if (devicesActGrp->checkedAction() == 0 && menuDevices->actions().length() > 0)
        menuDevices->actions().first()->setChecked(true);
}

void MainWindow::deviceClicked()
{
    if (appState == STATE_STREAMING || appState == STATE_CONNECTING_DEVICE)
        setState(STATE_CONNECTING_DEVICE);
}

void MainWindow::loadLicense(QString fileName)
{
    if (prefs->loadLicense(fileName))
    {
        setWindowTitle("StreamPlay");
        prefDialog->enableTabs();
        prefDialog->setRegState();
        prefDialog->showLicenseTab();
        prefDialog->show();
        prefDialog->activateWindow();
        prefDialog->raise();
        prefDialog->setFocus();

        // resume playback
        if (airplay && airplay->getStatus() != AirPlay::CONNECTING)
            airplay->resume();
    }
}

MainWindow::~MainWindow()
{
    timerStateLoop.stop();
    if (hlsVideo) hlsVideo->stop();
    if (airplay) airplay->stop(); // stop play nicely
    if (bonjourResolver) delete bonjourResolver;
    delete ui;
}
