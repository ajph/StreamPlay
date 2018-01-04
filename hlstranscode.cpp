#include "hlstranscode.h"

#include <QFile>
#include <QDebug>

HlsTranscode::HlsTranscode(QString subs, QString ffmpp, QString streamPath, QObject *parent)
    : QObject(parent), ffmpegPath(ffmpp), streamPath(streamPath), subs(subs), readyForData(false)
{
    // start ffmpeg
    connect(&ffmpeg, SIGNAL(started()), this, SLOT(ffmpegStarted()));
    connect(&ffmpeg, SIGNAL(readyReadStandardOutput()), this, SLOT(segmentComplete()));
    connect(&ffmpeg, SIGNAL(readyReadStandardError()), this, SLOT(transcodeError()));
    connect(&ffmpeg, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
    connect(&ffmpeg, SIGNAL(finished(int)), this, SLOT(ffmpegTerminated(int)));
    startFfmpeg();
}

void HlsTranscode::startFfmpeg()
{
    QStringList args;
    args << "-i" << "pipe:0";

    // add subtitles
    if (subs.length() > 0)
        args << "-vf" << "subtitles="+subs+":charenc=Windows-1250";

    args << "-hide_banner";
    args << "-nostdin";
    args << "-nostats";
    args << "-loglevel" << "warning";
    args << "-c:v" << "libx264";
    args << "-profile:v" << "baseline" << "-level" << "3.1";
    args << "-pix_fmt" << "yuv420p";
    args << "-preset" << "ultrafast";
    args << "-crf" << "22";
    args << "-c:a" << "aac" << "-strict" << "-2";
    args << "-force_key_frames" << "expr:gte(t,n_forced*5)";
    args << "-f" << "ssegment";
    args << "-segment_format" << "mpegts";
    args << "-segment_list" << "pipe:1";
    args << "-segment_list_type" << "csv";
    args << "-segment_time" << "10";
    args << streamPath + "/a%03d.ts";

    qDebug() << "start ffmpeg";
    qDebug() << ffmpegPath << args;

    // start
    pendingBytes = 0;
    completedSegments = 0;
    ffmpeg.start(ffmpegPath, args);
}

void HlsTranscode::transcodeError()
{
    QByteArray err = ffmpeg.readAllStandardError();
    if (err.length() > 0)
        qDebug() << err;
}

void HlsTranscode::segmentComplete()
{
    // parse output
    while (ffmpeg.canReadLine())
    {
        QString ln = ffmpeg.readLine().trimmed();
        QStringList s = ln.split(",");
        if (s.length() != 3)
            continue;
        completedSegments++;
        emit segmentAvailable(completedSegments - 1);
    }
}

void HlsTranscode::ffmpegStarted()
{
    readyForData = true;
}

void HlsTranscode::stop()
{
    readyForData = false;
    if (ffmpeg.isOpen())
        ffmpeg.close();
}

void HlsTranscode::pipeInput(QByteArray *in)
{
    if (!ffmpeg.isOpen()) return;
    readyForData = false;
    pendingBytes += in->size();
    ffmpeg.write(*in);
}

void HlsTranscode::bytesWritten(qint64 sz)
{
    pendingBytes -= sz;
    readyForData = (pendingBytes == 0);
}

void HlsTranscode::ffmpegTerminated(int code)
{
    readyForData = false;
    qDebug() << ffmpeg.readAllStandardError() << ffmpeg.readAllStandardOutput();
    qDebug() << "ffmpeg terminated:" + QString::number(code);
}

HlsTranscode::~HlsTranscode()
{    
    stop();    
}

