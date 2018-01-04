#include "hlsvideo.h"
#include "hlsserver.h"
#include "hlstranscode.h"

#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QDebug>

HlsVideo::HlsVideo(QString streamPath, QObject *parent)
    : QObject(parent), hlsServer(), hlsTranscode(), streamPath(streamPath)
{
}

void HlsVideo::start(QString file, QString subs, int port)
{    
    httpPort = port;
    videoFile = file;
    srtFile = subs;

    // start server & transcoder
    hlsServer = new HlsServer(videoFile, getFfmpegPath(), streamPath, 8888, this);
    hlsTranscode = new HlsTranscode(srtFile, getFfmpegPath(), streamPath, this);
    connect(hlsTranscode, SIGNAL(segmentAvailable(int)), hlsServer, SLOT(segmentAvailable(int)));
    connect(hlsTranscode, SIGNAL(segmentAvailable(int)), this, SLOT(segmentAvailable(int)));
}

/*void HlsVideo::stateLoop()
{
    switch (state)
    {
    case STATE_START_SERVER:
    {
        // start services
        hlsServer = new HlsServer(videoFile, getFfmpegPath(), streamPath, 8888, this);
        hlsTranscode = new HlsTranscode(srtFile, getFfmpegPath(), streamPath, this);
        connect(hlsTranscode, SIGNAL(segmentAvailable(int)), hlsServer, SLOT(segmentAvailable(int)));
        connect(hlsTranscode, SIGNAL(readyForData()), this, SIGNAL(readyForData()));
        state = STATE_FLUSH_BUFFER;
        break;
    }  
    case STATE_FLUSH_BUFFER:
    {
        // write buffer
        if (buffer.length() > 0)
        {
            hlsTranscode->pipeInput(&buffer);
            buffer.clear();
        }
        //

        // is complete?
        if (hlsServer->getTotalSegments() > 0 &&
                hlsTranscode->getTotalSegments() >= hlsServer->getTotalSegments()-2)
        {
            qDebug() << "transcode complete";
            state = STATE_COMPLETE;
        }
        break;
    }
    case STATE_COMPLETE:
    {
        // make last segment available
        hlsServer->segmentAvailable(hlsServer->getTotalSegments()-1);

        // finish
        hlsTranscode->stop();
        timerStateLoop.stop();
    }
    default:
        break;
    }
}*/

QString HlsVideo::getFfmpegPath()
{
#ifdef QT_DEBUG
    return "/Users/ajph/Documents/projects/StreamPlay/src/ffmpeg/ffmpeg";
#else
    return QCoreApplication::applicationDirPath() + "/../Resources/ffmpeg";
#endif
}

void HlsVideo::encode(QByteArray *b)
{
    hlsTranscode->pipeInput(b);
}

bool HlsVideo::isReadyForStreaming()
{
    return ((hlsTranscode != NULL) &&
            (hlsTranscode->getCompletedSegments() > 2));
}

bool HlsVideo::isReadyForInput()
{
    return ((hlsTranscode != NULL) &&
            (hlsTranscode->isReadyForData()));
}

int HlsVideo::getCurrentSegment() {
    if (!hlsServer)
        return 0;
    else
        return hlsServer->getCurrentSegment();
}

int HlsVideo::getTotalSegments() {
    if (!hlsServer)
        return 0;
    else
        return hlsServer->getTotalSegments();
}

void HlsVideo::segmentAvailable(int i)
{
    if (hlsServer->getTotalSegments() > 0 && i >= hlsServer->getTotalSegments()-2)
    {
        qDebug() << "transcode complete";
        hlsServer->segmentAvailable(hlsServer->getTotalSegments()-1);
        hlsTranscode->stop();
    }
}

void HlsVideo::stop()
{    
    if (hlsServer)
        hlsServer->pause();
    if (hlsTranscode)
        hlsTranscode->stop();
}

HlsVideo::~HlsVideo()
{
    stop();
}

