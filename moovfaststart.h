#ifndef MOOVFASTSTART_H
#define MOOVFASTSTART_H

#include <QObject>
#include <QVector>

class moovFastStart : public QObject
{
    Q_OBJECT
public:
    explicit moovFastStart(QByteArray *moov, QObject *parent = 0);
    ~moovFastStart();
    QByteArray* offsetStcos();

    struct Atom {
        int index;
        quint32 name;
        quint64 offset;
        quint64 sz;
    };

signals:

public slots:

private:
    QVector<quint64> findStcos();
    QByteArray* moov;
};

#endif // MOOVFASTSTART_H
