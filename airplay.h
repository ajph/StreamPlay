#ifndef AIRPLAY_H
#define AIRPLAY_H

#include <QObject>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QTimer>

class QTcpSocket;
class QHostInfo;

class AirPlay : public QObject
{
    Q_OBJECT

public:
    struct Position {
        float duration;
        float position;
    };

    enum state {
        CONNECTING,
        CONNECTED,
        PLAYING,
        PAUSED
    };

    explicit AirPlay(QString host, int port, QObject *parent = 0);
    ~AirPlay();
    state getStatus() {
        return status;
    }
    Position getPosition() {
        return position;
    }

    QHostAddress getLocalIp() {
        return localIp;
    }

    bool play(QString url);
    void stop();
    void pause();
    void resume();

private:
    state status;
    QTcpSocket *socket;
    QString host;
    int port;
    QTimer timerPos, timerTryConnect;
    Position position;
    QHostAddress localIp;    

signals:

public slots:

private slots:
    void sockStatus(QAbstractSocket::SocketState s);
    void pos();
    void tryConnect();
};

#endif // AIRPLAY_H
