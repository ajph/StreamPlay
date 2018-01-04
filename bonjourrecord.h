#ifndef BonjourRecord_H
#define BonjourRecord_H

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QHostInfo>

class BonjourBrowseRecord
{
public:
    BonjourBrowseRecord() {}
    BonjourBrowseRecord(const quint32 index, const QString &name, const QString &regType, const QString &domain)
        : interfaceIndex(index), serviceName(name), registeredType(regType), replyDomain(domain)
    {}
    BonjourBrowseRecord(const quint32 index, const char *name, const char *regType, const char *domain)
    {
        interfaceIndex = index;
        serviceName = QString::fromUtf8(name);
        registeredType = QString::fromUtf8(regType);
        replyDomain = QString::fromUtf8(domain);
    }
    quint32 interfaceIndex;
    QString serviceName;
    QString registeredType;
    QString replyDomain;
    bool operator==(const BonjourBrowseRecord &other) const {
        return interfaceIndex == other.interfaceIndex
               && serviceName == other.serviceName
               && registeredType == other.registeredType
               && replyDomain == other.replyDomain;
    }
};

Q_DECLARE_METATYPE(BonjourBrowseRecord)

#endif // BonjourBrowseRecord_H
