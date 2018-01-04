#ifndef HLSVIDEO_H
#define HLSVIDEO_H

#include <QObject>
#include <QTimer>
#include <QBuffer>

class HlsServer;
class HlsTranscode;

class HlsVideo : public QObject
{
    Q_OBJECT
public:
    explicit HlsVideo(QString streamPath, QObject *parent = 0);
    ~HlsVideo();
    void start(QString videoFile, QString srtFile, int port);
    bool isReadyForStreaming();
    bool isReadyForInput();
    void encode(QByteArray *b);
    int getCurrentSegment();
    int getTotalSegments();
    void stop();

private slots:
    void segmentAvailable(int);

private:
    QString getFfmpegPath();

    HlsServer *hlsServer;
    HlsTranscode *hlsTranscode;
    QString videoFile;
    int httpPort;
    QString streamPath;
    QString srtFile;

signals:

};

#endif // HLSVIDEO_H
