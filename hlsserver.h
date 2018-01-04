#ifndef HLSSERVER_H
#define HLSSERVER_H


#include <QObject>
#include <QTcpServer>
#include <QProcess>
#include <QHash>
#include <QPointer>

class HlsServer : public QTcpServer
{
    Q_OBJECT

public:
    HlsServer(QString videoFile, QString ffmpegPath, QString streamPath, quint16 port, QObject* parent = 0);
    ~HlsServer();
    void incomingConnection(qintptr socket);
    void pause();
    void resume();
    int getTotalSegments() {
        return totalSegments;
    }
    int getCurrentSegment() {
        return currentSegment;
    }

public slots:
    void segmentAvailable(int);

signals:

private slots:
    void readClient();
    void discardClient();

private:
    void writeSegment(int seg, QTcpSocket *socket);
    void buildPlaylist();
    qint64 getLengthMs(int *error_code);
    bool disabled;
    QString m3u8;
    int currentSegment;
    QHash<int, QPointer<QTcpSocket> > waitingSockets;
    QString videoFile;
    QString ffmpegPath;
    qint64 totalSegments;
    QString streamPath;
};


#endif // HLSSERVER_H
