#ifndef PXMSYNC_H
#define PXMSYNC_H

#include <QWidget>
#include <QMap>
#include <QUuid>

#include <event2/util.h>

#include <pxmdefinitions.h>

class PXMSync : public QObject
{
    Q_OBJECT
    QHash<QUuid, peerDetails> *syncHash;
public:
    PXMSync(QObject *parent);
    void setsyncHash(QHash<QUuid, peerDetails> *hash);
    QHash<QUuid, peerDetails>::iterator hashIterator;
    void setIteratorToStart();
public slots:
    void syncNext();
signals:
    void requestIps(BevWrapper*, QUuid);
    void syncComplete();
};

#endif // MESSSYNC_H
