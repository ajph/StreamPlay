#include "airplay.h"

#include <QTcpSocket>
#include <QTimer>

AirPlay::AirPlay(QString h, int p, QObject *parent) : QObject(parent)
{    
    position.duration = 0.0;
    position.position = 0.0;
    host = h;
    port = p;

    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(sockStatus(QAbstractSocket::SocketState)));

    status = CONNECTING;
    connect(&timerTryConnect, SIGNAL(timeout()), this, SLOT(tryConnect()));
    timerTryConnect.start(5000);
    tryConnect();
}

void AirPlay::tryConnect()
{
    // test connection
    qDebug() << "attempting connection to" << host << port;
    socket->abort();
    socket->connectToHost(host, port);
}

bool AirPlay::play(QString url)
{
    if (status == CONNECTING) return false;

    QString body;
    body.append("Content-Location: "+url+"\n");
    body.append("Start-Position: 0.0\n");

    QTextStream os(socket);
    os.setAutoDetectUnicode(true);
    os << "POST /play HTTP/1.1" << "\r\n";
    os << "Content-Length: " << QString::number(body.length()) << "\r\n";
    os << "\r\n";
    os << body;
    os.flush();

    status = PLAYING;

    return true;
}

void AirPlay::stop()
{
    if (status == CONNECTING) return;

    timerPos.stop();

    socket->readAll(); // clear buffer
    QTextStream os(socket);
    os.setAutoDetectUnicode(true);
    os << "POST /stop HTTP/1.1" << "\r\n";
    os << "\r\n";
    os.flush();
    socket->waitForReadyRead(1000);

    // reset positon
    position.duration = 0.0;
    position.position = 0.0;

    // close socket
    socket->close();
}

void AirPlay::pause()
{
    if (status == CONNECTING) return;

    socket->readAll(); // clear buffer
    QTextStream os(socket);
    os.setAutoDetectUnicode(true);
    os << "POST /rate?value=0.0 HTTP/1.1" << "\r\n";
    os << "\r\n";
    os.flush();
    socket->waitForReadyRead(1000);
    status = PAUSED;
}

void AirPlay::resume()
{
    if (status == CONNECTING) return;

    socket->readAll(); // clear buffer
    QTextStream os(socket);
    os.setAutoDetectUnicode(true);
    os << "POST /rate?value=1.0 HTTP/1.1" << "\r\n";
    os << "\r\n";
    os.flush();
    socket->waitForReadyRead(1000);
    status = PLAYING;
}

void AirPlay::pos()
{
    if (status == CONNECTING) return;

    socket->readAll(); // clear buffer

    QTextStream os(socket);
    os.setAutoDetectUnicode(true);
    os << "GET /scrub HTTP/1.1" << "\r\n";
    os << "\r\n";
    os.flush();

    // read response
    if (socket->waitForReadyRead(1000))
    {
        while(socket->canReadLine())
        {
            QString ln = socket->readLine().trimmed();
            if (ln.startsWith("duration:"))
            {
                int i = ln.indexOf(":");
                position.duration = ln.mid(i+1).trimmed().toInt();
            }
            else if (ln.startsWith("position:"))
            {
                int i = ln.indexOf(":");
                position.position = ln.mid(i+1).trimmed().toInt();
            }
        }
    }
}

void AirPlay::sockStatus(QAbstractSocket::SocketState s) {
    switch(s)
    {
    case QAbstractSocket::ClosingState:
        if (status != CONNECTING) {
            timerPos.stop();
            break;
        }
    case QAbstractSocket::ConnectedState:
        timerTryConnect.stop();

        // get local ip
        localIp = socket->localAddress();

        // position timer
        connect(&timerPos, SIGNAL(timeout()), this, SLOT(pos()));
        timerPos.start(1000);

        status = CONNECTED;
        break;
    default:
        break;
    }
}

AirPlay::~AirPlay()
{
    timerTryConnect.stop();
    stop();
    timerPos.stop();
    if (socket->isOpen()) socket->close();
    socket->deleteLater();
}
