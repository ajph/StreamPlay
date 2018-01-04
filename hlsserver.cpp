#include "hlsserver.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>
#include <QFile>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QApplication>
#include <QtCore/qmath.h>
#include <QDebug>

HlsServer::HlsServer(QString videoFile, QString ffmpegPath, QString streamPath, quint16 port, QObject* parent)
    : QTcpServer(parent), disabled(false), videoFile(videoFile), ffmpegPath(ffmpegPath), streamPath(streamPath)
{
    m3u8.clear();
    currentSegment = -1;
    totalSegments = 0;

    // start webserver listen
    listen(QHostAddress::Any, port);
    // if (!successful)
    //      throw 1;
}

void HlsServer::incomingConnection(qintptr socket)
{
    if (disabled)
        return;

    // When a new client connects, the server constructs a QTcpSocket and all
    // communication with the client is done over this QTcpSocket. QTcpSocket
    // works asynchronously, this means that all the communication is done
    // in the two slots readClient() and discardClient().
    QTcpSocket* s = new QTcpSocket(this);
    connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
    connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
    s->setSocketDescriptor(socket);
}

void HlsServer::readClient()
{
    if (disabled)
        return;

    // This slot is called when the client has sent data to the server
    QTcpSocket* socket = (QTcpSocket*)sender();

    // get request
    QStringList request;
    if (socket->canReadLine())
        request = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"), QString::SkipEmptyParts);

    qDebug() << request;

    // build response
    QString response = "HTTP/1.1 200 OK\r\n";
    if (request[1].endsWith(".m3u8"))
    {
        if (m3u8.length() == 0)
            buildPlaylist();
        response.append("Content-Type: application/vnd.apple.mpegurl\r\n");
        response.append("\r\n");
        response.append(m3u8); // output playlist
    }
    else if (request[1].endsWith(".ts"))
    {
        response.append("Content-Type: video/MP2T\r\n");
        response.append("\r\n");
    }

    // output response & headers
    QTextStream os(socket);
    os.setAutoDetectUnicode(true);
    os << response;
    os.flush();

    if (request[1].endsWith(".m3u8"))
    {
        socket->close();
    }
    else if (request[1].endsWith(".ts"))
    {
        int rqSeg = request[1].mid(2, request[1].indexOf(".") - 2).toInt();
        if (currentSegment >= rqSeg)
        {            
            // write segment immediately
            writeSegment(rqSeg, socket);
            socket->close();
        }
        else
        {
            // queue segment
            if (!waitingSockets.contains(rqSeg))
                waitingSockets.insert(rqSeg, socket);
        }
    }
}

qint64 HlsServer::getLengthMs(int *error_code)
{
    *error_code = 0;

    QProcess ff;
    QString prog = ffmpegPath;
    QStringList args(QStringList() << "-hide_banner" << "-i" << videoFile);
    ff.start(prog, args);
    ff.waitForReadyRead();

    // process output
    ff.setReadChannel(QProcess::StandardError);
    while (ff.canReadLine())
    {
        QString ln = ff.readLine().trimmed();
        if (ln.startsWith("Duration: "))
        {
            int s = 10;
            int f = ln.indexOf(", start: ");
            QStringList s_dur = ln.mid(s, f-s).split(":");
            if (s_dur.length() != 3) {
                *error_code = 1;
                return 0;
            }
            return (s_dur.at(0).toInt() * 3600000)
                    + (s_dur.at(1).toInt() * 60000)
                    + (s_dur.at(2).toFloat() * 1000);
            break;
        }
    }

    *error_code = 2;
    return 0;
}

void HlsServer::buildPlaylist()
{
    int ec;
    qint64 ms = getLengthMs(&ec);
    if (ec > 0)
    {
        qDebug() << "error reading length";
        return;
    }
    qDebug() << "duration" << ms << "ms";

    // playlsit header
    m3u8.append("#EXTM3U\r\n");
    m3u8.append("#EXT-X-PLAYLIST-TYPE:VOD\r\n");
    m3u8.append("#EXT-X-TARGETDURATION:10\r\n");
    m3u8.append("#EXT-X-VERSION:3\r\n");
    m3u8.append("#EXT-X-MEDIA-SEQUENCE:0\r\n");

    // playlist segments
    totalSegments = qFloor(((float)ms / 1000.0) / 10.0);
    int i = 0;
    for (i=0;i<totalSegments;i++)
    {
        m3u8.append("#EXTINF:10.0,\r\n");
        m3u8.append(QString("a%1.ts\r\n").arg(i, 3, 10, QChar('0')));
    }
    int lastSegLength = (ms / 1000) % 10;
    if (lastSegLength > 0)
    {
        m3u8.append("#EXTINF:" + QString::number(lastSegLength) + ",\r\n");
        m3u8.append(QString("a%1.ts\r\n").arg(i, 3, 10, QChar('0')));
        totalSegments++;
    }

    // end playlist
    m3u8.append("#EXT-X-ENDLIST\r\n");
}

void HlsServer::writeSegment(int seg, QTcpSocket *socket)
{
    QString sSeg = QString("a%1.ts").arg(seg, 3, 10, QChar('0'));
    QFile vf(streamPath + "/" + sSeg);
    if (!vf.exists())
    {
        qDebug() << sSeg << " does not exist!";
        return;
    }
    vf.open(QIODevice::ReadOnly);
    QDataStream ds(socket);
    ds.writeRawData(vf.readAll().data(), vf.size());
    vf.close();
}

void HlsServer::segmentAvailable(int s)
{
    currentSegment = s;
    qDebug() << "segment" << s << "done";

    // process waiting segments
    if (waitingSockets.contains(s))
    {
        QTcpSocket *socket = waitingSockets.value(s);
        if (socket && socket->isValid())
        {
            writeSegment(s, socket);
            socket->close();
        }
        waitingSockets.remove(s);
    }
}

void HlsServer::discardClient()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    socket->deleteLater();
}

void HlsServer::pause()
{
    disabled = true;
}

void HlsServer::resume()
{
    disabled = false;
}

HlsServer::~HlsServer()
{
    if (isListening()) close();
}
