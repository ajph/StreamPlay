#include "moovfaststart.h"

#include <QDataStream>
#include <QFile>
#include <QDebug>

moovFastStart::moovFastStart(QByteArray *moov, QObject *parent)
    : QObject(parent), moov(moov)
{
}

QVector<quint64> moovFastStart::findStcos()
{
    QVector<quint64> ret;
    QDataStream ds(*moov);
    qDebug() << "finding stcos";
    for (int i=0; !ds.atEnd(); i++)
    {
        ds.device()->seek(i);
        if (ds.device()->read(4) == "stco")
        {
            qint64 pos = ds.device()->pos() - 8;
            qDebug() << "found stco @" << hex << pos;
            ret.append(pos);
        }
    }
    return ret;
}

QByteArray* moovFastStart::offsetStcos()
{
    QDataStream ds(moov, QIODevice::ReadWrite);

    QVector<quint64> offsets = findStcos();
    qDebug() << "changing" << offsets.length() << "offsets";
    for (QVector<quint64>::Iterator ofst = offsets.begin(); ofst != offsets.end(); ++ofst)
    {
        ds.device()->reset();
        ds.skipRawData(*ofst + 12); // size + name + version
        quint32 entries = 0;
        ds >> entries;
        for (quint32 i=0; i<entries; i++)
        {
            quint32 sz = 0;
            ds >> sz;
            ds.skipRawData(-4);
            ds << sz + moov->length();
        }
    }

    return moov;
}

moovFastStart::~moovFastStart()
{

}

