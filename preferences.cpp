#include "preferences.h"

#include <QDir>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

const QByteArray s1 = "__StreamPlay@";
const QByteArray s2 = "QStringQByteArrayQObject";

Preferences::Preferences(QObject *parent)
    : QObject(parent), recentFiles()
{

#ifdef QT_DEBUG
    s.clear();
#endif

    // set up first-run settings
    if (!s.contains("inbuilt/hasrun") || !s.value("inbuilt/hasrun").toBool())
    {
        qDebug() << "first run";
        setSavePath( QDir::tempPath() + "/streamplay" );
        setStreamPath( QDir::tempPath() + "/streamplay/stream" );
        setSaveTorrentVideos(false);
        setBittorrentPort(2323);
        s.remove("user");
        s.setValue("inbuilt/hasrun", true);
    }

    ch1();
}

// save path

QString Preferences::savePath()
{
    return s.value("storage/savepath").toString();
}

void Preferences::setSavePath(QString path)
{
    s.setValue("storage/savepath", QDir().cleanPath(path));
}

// stream path

QString Preferences::streamPath()
{
    return s.value("storage/streampath").toString();
}

void Preferences::setStreamPath(QString path)
{
    s.setValue("storage/streampath", QDir().cleanPath(path));
}

// recently opened files

QVector<Preferences::RecentFile>* Preferences::recentlyOpenedFiles()
{
    if (!recentFiles)
    {
        int sz = s.beginReadArray("recentfiles");
        recentFiles = new QVector<Preferences::RecentFile>;
        for (int i=0;i<sz;i++)
        {
            s.setArrayIndex(i);
            Preferences::RecentFile rf;
            rf.name = s.value("name").toString();
            rf.path = s.value("path").toString();
            recentFiles->append(rf);
        }
        s.endArray();
        emit recentFilesUpdated();
    }
    return recentFiles;
}

void Preferences::addRecentlyOpenedFile(QString name, QString path)
{
    recentlyOpenedFiles();

    RecentFile rf;
    rf.name = name;
    rf.path = path;
    recentFiles->prepend(rf);

    for (QVector<Preferences::RecentFile>::iterator it = recentFiles->begin()+1;
         it != recentFiles->end(); )
    {
        if (rf.name == it->name)
            it = recentFiles->erase(it);
        else
            ++it;
    }

    // remove last if > 20 items
    while (recentFiles->length() > 20)
        recentFiles->erase(recentFiles->end()-1);

    // repopulate settings
    s.remove("recentfiles");
    s.beginWriteArray("recentfiles");
    for (int i=0;i<recentFiles->length();i++)
    {
        s.setArrayIndex(i);
        s.setValue("name", recentFiles->at(i).name);
        s.setValue("path", recentFiles->at(i).path);
    }
    s.endArray();

    emit recentFilesUpdated();
}

bool Preferences::loadLicense(QString fileName)
{
    // open file
    QFile lf(fileName);
    if (!lf.open(QIODevice::ReadOnly))
        return false;

    // read contents into json document
    QJsonDocument doc = QJsonDocument::fromJson(lf.readAll());
    lf.close();
    if (doc.isNull() || !doc.isObject())
        return false;

    // check required fields
    QJsonObject obj = doc.object();
    if (!obj.contains("name") ||
            !obj.contains("email") ||
            !obj.contains("order") ||
            !obj.contains("created") ||
            !obj.contains("description") ||
            !obj.contains("signature"))
        return false;

    // fields ok, add to settings
    s.setValue("user/name", obj.value("name").toString().trimmed());
    s.setValue("user/email", obj.value("email").toString().trimmed());
    s.setValue("user/order", obj.value("order").toString().trimmed());
    s.setValue("user/created", obj.value("created").toString().trimmed());
    s.setValue("user/description", obj.value("description").toString().trimmed());
    s.setValue("user/sig", obj.value("signature").toString().trimmed());

    ch3();

    if (cch == false)
        s.remove("user");

    return true;
}

QString Preferences::regName()
{
    return s.value("user/name").toString();
}

QString Preferences::regEmail()
{
    return s.value("user/email").toString();
}

QString Preferences::regOrder()
{
    return s.value("user/order").toString();
}

void Preferences::clearRecentlyOpenedFiles()
{
    recentlyOpenedFiles();

    s.remove("recentfiles");
    recentFiles->clear();
    emit recentFilesUpdated();
}

// save torrents

bool Preferences::saveTorrentVideos()
{
    return s.value("storage/savetorrentvideos").toBool();
}

void Preferences::setSaveTorrentVideos(bool b)
{
    s.setValue("storage/savetorrentvideos", b);
}

// bt port

int Preferences::bittorrentPort()
{
    return s.value("bittorrent/port").toInt();
}

void Preferences::setBittorrentPort(int port)
{
    s.setValue("bittorrent/port", port);
}

void Preferences::ch1()
{
    // check settings
    if (!s.contains("user/name") ||
            !s.contains("user/email") ||
            !s.contains("user/order") ||
            !s.contains("user/created") ||
            !s.contains("user/description") ||
            !s.contains("user/sig"))
    {
        cch = false;
        return;
    }

    // read settings
    QString name = s.value("user/name").toString().trimmed();
    QString email = s.value("user/email").toString().trimmed();
    QString order = s.value("user/order").toString().trimmed();
    QString created = s.value("user/created").toString().trimmed();
    QString description = s.value("user/description").toString().trimmed();
    QString sig = s.value("user/sig").toString().trimmed();

    // generate salt
    QByteArray s3 = "";
    for(int i=0; i < s1.length(); i++)
        s3.append(s1[i] ^ s2[i]);

    // hash
    QByteArray h(s3);
    h.append(name);
    h.append(email);
    h.append(order);
    h.append(created);
    h.append(description);
    QByteArray cSig = QCryptographicHash::hash(h, QCryptographicHash::Sha512);

    // compare return
    cch = (cSig.toBase64().toBase64() == sig);
}

void Preferences::ch2()
{
    // check settings
    if (!s.contains("user/name") ||
            !s.contains("user/email") ||
            !s.contains("user/order") ||
            !s.contains("user/created") ||
            !s.contains("user/description") ||
            !s.contains("user/sig"))
    {
        cch = false;
        return;
    }

    // read settings
    QString name = s.value("user/name").toString().trimmed();
    QString email = s.value("user/email").toString().trimmed();
    QString order = s.value("user/order").toString().trimmed();
    QString created = s.value("user/created").toString().trimmed();
    QString description = s.value("user/description").toString().trimmed();
    QString sig = s.value("user/sig").toString().trimmed();

    // generate salt
    QByteArray s3 = "";
    for(int i=0; i < s1.length(); i++)
        s3.append(s1[i] ^ s2[i]);

    // hash
    QByteArray h(s3);
    h.append(name);
    h.append(email);
    h.append(order);
    h.append(created);
    h.append(description);
    QByteArray cSig = QCryptographicHash::hash(h, QCryptographicHash::Sha512);

    // compare return
    cch = (cSig.toBase64().toBase64() == sig);
}

void Preferences::ch3()
{
    // check settings
    if (!s.contains("user/name") ||
            !s.contains("user/email") ||
            !s.contains("user/order") ||
            !s.contains("user/created") ||
            !s.contains("user/description") ||
            !s.contains("user/sig"))
    {
        cch = false;
        return;
    }

    // read settings
    QString name = s.value("user/name").toString().trimmed();
    QString email = s.value("user/email").toString().trimmed();
    QString order = s.value("user/order").toString().trimmed();
    QString created = s.value("user/created").toString().trimmed();
    QString description = s.value("user/description").toString().trimmed();
    QString sig = s.value("user/sig").toString().trimmed();

    // generate salt
    QByteArray s3 = "";
    for(int i=0; i < s1.length(); i++)
        s3.append(s1[i] ^ s2[i]);

    // hash
    QByteArray h(s3);
    h.append(name);
    h.append(email);
    h.append(order);
    h.append(created);
    h.append(description);
    QByteArray cSig = QCryptographicHash::hash(h, QCryptographicHash::Sha512);

    // compare return
    cch = (cSig.toBase64().toBase64() == sig);
}

Preferences::~Preferences()
{
    qDebug() << "running prefs destructor";
    if (recentFiles) delete recentFiles;

    // clear stream path
    QDir(streamPath()).removeRecursively();

    // clear save path
    if (!saveTorrentVideos())
        QDir(savePath()).removeRecursively();
}
