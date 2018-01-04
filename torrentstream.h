#ifndef TORRENTSTREAM_H
#define TORRENTSTREAM_H

#include <libtorrent/torrent_handle.hpp>

#include <QObject>
#include <QTimer>

class TorrentStream : public QObject
{
    Q_OBJECT

public:
    enum state {
        STATE_IDLE,
        STATE_ADDING_TORRENT,
        STATE_RETRIEVING_METADATA,
        STATE_BUFFERING,
        STATE_READY
    };
    enum errorCode {
        ERR_READING_TORRENT_FILE,
        ERR_ADDING_TORRENT,
        ERR_TORRENT_NOT_VIDEO,
        ERR_TIMEOUT,
        ERR_DOWNLOAD,
        ERR_UNKNOWN
    };
    struct Error {
        errorCode errorType;
        QString errorString;
    };
    struct Stats {
        QString name;
        qint64 bytes_done;
        float progress;
        float dSpeed;
    };

    explicit TorrentStream(QString savePath, int incomingPort, QObject *parent = 0);
    ~TorrentStream();
    void start(QString t);
    Stats getStats();
    state getStatus();
    Error getError();
    QString getFilePath();
    quint64 getFileOffset();
    bool hasPiece(int i);
    bool hasBytes(quint64 from, quint64 to);
    void prioritizeBytes(quint64 from, quint64 len);
    quint64 getPieceLength();
    quint64 getFileSize();

private:
    void setState(state s);
    void setError(errorCode e, QString s);
    libtorrent::session *s;
    libtorrent::torrent_handle h;
    QTimer timerUpdate;
    state status;
    Error error;
    libtorrent::file_entry targetFile;
    boost::intrusive_ptr<libtorrent::torrent_info const> torrentInfo;
    QString savePath;
    int incomingPort;

signals:
    void statusChange(TorrentStream::state s);
    void errored(Error e);

public slots:

private slots:
    void update();
};

#endif // TORRENTSTREAM_H
