#include "torrentstream.h"

#include <libtorrent/entry.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/extensions/ut_metadata.hpp>
#include <libtorrent/extensions/ut_pex.hpp>
#include <libtorrent/extensions/smart_ban.hpp>

#include <QDebug>

TorrentStream::TorrentStream(QString savePath, int incomingPort, QObject *parent) : QObject(parent),
    s(), savePath(savePath), incomingPort(incomingPort)
{
    // new session
    s = new libtorrent::session(libtorrent::fingerprint("UT", 3, 3, 0, 0));

    // enable extensions
    s->add_extension(&libtorrent::create_ut_metadata_plugin);
    s->add_extension(&libtorrent::create_ut_pex_plugin);
    s->add_extension(&libtorrent::create_smart_ban_plugin);

    // listen
    libtorrent::error_code ec;
    s->listen_on(std::make_pair(incomingPort, incomingPort), ec, 0, libtorrent::session::listen_no_system_port);
    if (ec)
        throw 1;

    // settings
    libtorrent::session_settings settings = s->settings();
    settings.user_agent = "uTorrent/3300(29677)";
    settings.request_timeout = 5;
    settings.peer_connect_timeout = 2;
    settings.announce_to_all_trackers = true;
    settings.announce_to_all_tiers= true;
    settings.connection_speed = 20;
    settings.torrent_connect_boost = 20;
    settings.no_connect_privileged_ports = false;
    settings.stop_tracker_timeout = 1;

    // don't use any disk cache
    settings.cache_size = 0;
    settings.cache_buffer_chunk_size = 1;
    settings.use_read_cache = false;
    settings.use_disk_read_ahead = false;

    s->set_settings(settings);

    // encryption settings
    libtorrent::pe_settings encSettings = libtorrent::pe_settings();
    encSettings.out_enc_policy = libtorrent::pe_settings::enabled;
    encSettings.in_enc_policy = libtorrent::pe_settings::enabled;
    encSettings.allowed_enc_level = libtorrent::pe_settings::both;
    encSettings.prefer_rc4 = true;
    s->set_pe_settings(encSettings);

    // start services
    s->start_upnp();
    s->start_natpmp();
    s->start_dht();
    s->add_dht_router(std::make_pair(std::string("router.bittorrent.com"), 6881));
    s->add_dht_router(std::make_pair(std::string("router.utorrent.com"), 6881));
    s->add_dht_router(std::make_pair(std::string("dht.transmissionbt.com"), 6881));
    s->add_dht_router(std::make_pair(std::string("dht.aelitis.com"), 6881)); // Vuze

    // init update timer
    connect(&timerUpdate, SIGNAL(timeout()), this, SLOT(update()));

    status = STATE_IDLE;
}

void TorrentStream::start(QString t)
{
    // reset state
    if (status != STATE_IDLE)
    {
        if (timerUpdate.isActive()) timerUpdate.stop();
        s->remove_torrent(h);
    }
    setState(STATE_ADDING_TORRENT);

    libtorrent::error_code ec;

    // set parameters
    libtorrent::add_torrent_params p;
    p.auto_managed = false;
    p.paused = true;
    p.duplicate_is_error = false;
    p.save_path = savePath.toStdString();

    // magnet or url
    if (t.startsWith("http") || t.startsWith("magnet:"))
    {
        p.url = t.toStdString();
    }
    else
    {
        p.ti = new libtorrent::torrent_info(t.toStdString(), ec);
        if (ec)
        {
            setError(ERR_READING_TORRENT_FILE, QString::fromStdString(ec.message()));
            return;
        }
    }

    // add the torrent
    h = s->add_torrent(p, ec);
    if (ec)
    {
        setError(ERR_ADDING_TORRENT, QString::fromStdString(ec.message()));
        return;
    }

    // start torrent as sequential download
    h.set_sequential_download(true);
    h.resume();
    setState(STATE_RETRIEVING_METADATA);

    // start state loop
    timerUpdate.start(1000);
}

void TorrentStream::update()
{
    libtorrent::torrent_status torrentStatus = h.status();
    if (torrentStatus.error != "")
    {
        setError(ERR_DOWNLOAD, QString::fromStdString(torrentStatus.error));
        timerUpdate.stop();
        return;
    }

    switch (status)
    {
    case STATE_RETRIEVING_METADATA:
        // next state
        if (torrentStatus.has_metadata)
        {
            // get torrent info
            torrentInfo = h.torrent_file();
            if (torrentInfo == NULL)
            {
                setError(ERR_ADDING_TORRENT, "unable to retrieve torrent info");
                timerUpdate.stop();
                return;
            }

            // find largest movie file
            int fileIndex = -1;
            libtorrent::size_type hi = 0;
            for (int i=0; i < torrentInfo->num_files(); ++i)
            {
                libtorrent::file_entry f = torrentInfo->file_at(i);
                QString path = QString::fromStdString(f.path);
                if (path.endsWith(".avi") || path.endsWith(".mkv") || path.endsWith(".mov")
                        || path.endsWith(".mp4") || path.endsWith(".m4a") || path.endsWith(".wmv"))
                {
                    if (hi == 0 || f.size > hi)
                    {
                        hi = f.size;
                        fileIndex = i;
                    }
                }
            }

            // none found?
            if (fileIndex == -1)
            {
                setError(ERR_TORRENT_NOT_VIDEO, "no video files found in torrent");
                timerUpdate.stop();
                return;
            }

            // set target file
            targetFile = torrentInfo->file_at(fileIndex);

            // set all other files to zero priority
            for (int i=0; i < torrentInfo->num_files(); ++i)
            {
                if (i == fileIndex)
                    continue;
                h.file_priority(i, 0);
            }  

            qDebug() << "pieces:" << torrentInfo->num_pieces();

            // set next state
            setState(STATE_BUFFERING);
        }
        break;

    case STATE_BUFFERING:
        // next state
        if (torrentStatus.state != libtorrent::torrent_status::checking_files)
        {
            if (torrentStatus.progress > 0.01)
                setState(STATE_READY);
        }
        break;

    case STATE_READY:
        timerUpdate.stop();
        break;

    default:
        setError(ERR_UNKNOWN, "invalid state");
        timerUpdate.stop();
        break;
    }
}

TorrentStream::Stats TorrentStream::getStats()
{
    Stats stats;
    if (h.is_valid())
    {
        libtorrent::torrent_status ts = h.status();
        stats.name = QString::fromStdString(ts.name);
        stats.progress = ts.progress;
        stats.dSpeed = ts.download_rate;
        stats.bytes_done = ts.total_wanted_done;
    }
    return stats;
}

QString TorrentStream::getFilePath()
{
    libtorrent::torrent_status ts = h.status();
    return QString::fromStdString(ts.save_path) + "/" + QString::fromStdString(targetFile.path);
}

quint64 TorrentStream::getFileSize()
{
    return targetFile.size;
}

quint64 TorrentStream::getFileOffset()
{
    return targetFile.offset;
}

bool TorrentStream::hasPiece(int i)
{
    return h.have_piece(i);
}

bool TorrentStream::hasBytes(quint64 from, quint64 len)
{
    int startPiece = (getFileOffset() + from) / getPieceLength();
    int endPiece = (getFileOffset() + from + len - 1) / getPieceLength();
    for (int i=startPiece; i<=endPiece; i++)
    {
        if (!hasPiece(i))
            return false;
    }
    return true;
}

void TorrentStream::prioritizeBytes(quint64 from, quint64 len)
{
    int startPiece = (getFileOffset() + from) / getPieceLength();
    int endPiece = (getFileOffset() + from + len - 1) / getPieceLength();
    for (int i=startPiece; i<=endPiece; i++)
    {
        h.piece_priority(i, 7);
    }
}

quint64 TorrentStream::getPieceLength()
{
    return torrentInfo->piece_length();
}

TorrentStream::Error TorrentStream::getError()
{
    return error;
}

TorrentStream::state TorrentStream::getStatus()
{    
    return status;
}

void TorrentStream::setState(state s)
{
    status = s;
    emit statusChange(status);
}

void TorrentStream::setError(errorCode e, QString s = "")
{
    error.errorType = e;
    error.errorString = s;
    emit errored(error);
}

TorrentStream::~TorrentStream()
{
    if (s) delete s;
}

