#ifndef HLSTRANSCODE_H
#define HLSTRANSCODE_H

#include <QObject>
#include <QProcess>

class HlsTranscode : public QObject
{
    Q_OBJECT
public:
    explicit HlsTranscode(QString subs, QString ffmpegPath, QString streamPath, QObject *parent = 0);
    ~HlsTranscode();
    int getCompletedSegments()
    {
        return completedSegments;
    }
    qint64 getPendingBytes()
    {
        return pendingBytes;
    }
    void pipeInput(QByteArray *in);
    bool isReadyForData() {
        return readyForData;
    }

    void stop();

private:
    void startFfmpeg();

    QProcess ffmpeg;
    QString ffmpegPath, streamPath, subs;
    int completedSegments;
    int pendingBytes;
    bool readyForData;

private slots:
    void segmentComplete();
    void transcodeError();
    void ffmpegTerminated(int code);
    void bytesWritten(qint64);
    void ffmpegStarted();

signals:
    void segmentAvailable(int);

};

#endif // HLSTRANSCODE_H
