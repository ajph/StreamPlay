#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>
#include <QSettings>
#include <QVector>

class Preferences : public QObject
{
    Q_OBJECT
public:
    struct RecentFile {
        QString path;
        QString name;
    };

    explicit Preferences(QObject *parent = 0);
    ~Preferences();

    QVector<RecentFile> *recentlyOpenedFiles();
    void addRecentlyOpenedFile(QString name, QString path);

    bool loadLicense(QString fileName);
    bool ch() {
        return cch;
    }
    void ch1();
    void ch2();
    void ch3();

    // getters
    QString savePath();
    QString streamPath();
    void clearRecentlyOpenedFiles();
    bool saveTorrentVideos();
    int bittorrentPort();
    QString regName();
    QString regEmail();
    QString regOrder();

    // setters
    void setSavePath(QString path);
    void setStreamPath(QString path);
    void setSaveTorrentVideos(bool b);
    void setBittorrentPort(int port);

public slots:

signals:
    void recentFilesUpdated();

private:
    QSettings s;
    QVector<RecentFile> *recentFiles;
    bool cch;

};

#endif // PREFERENCES_H
