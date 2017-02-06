#ifndef PXMPEERWORKER_H
#define PXMPEERWORKER_H

#include <QObject>
#include <QScopedPointer>

#include <event2/bufferevent.h>

#include "pxmpeers.h"
#include "pxmconsts.h"

class PXMPeerWorkerPrivate;

class PXMPeerWorker : public QObject
{
    Q_OBJECT
    QScopedPointer<PXMPeerWorkerPrivate> d_ptr;

   public:
    explicit PXMPeerWorker(QObject* parent,
                           QString username,
                           QUuid selfUUID,
                           QString multicast,
                           unsigned short tcpPort,
                           unsigned short udpPort,
                           QUuid globaluuid);
    ~PXMPeerWorker();
    PXMPeerWorker(PXMPeerWorker const&) = delete;
    PXMPeerWorker& operator=(PXMPeerWorker const&) = delete;
    PXMPeerWorker& operator=(PXMPeerWorker&&) noexcept = delete;
    PXMPeerWorker(PXMPeerWorker&&) noexcept            = delete;
    const int SYNC_TIMEOUT_MSECS                = 2000;
    const int SYNC_TIMER                        = 900000;
    QSharedPointer<unsigned char> createSyncPacket(size_t &index);
   public slots:
    int msgHandler(QString str, QUuid uuid, const bufferevent* bev,
                             bool global);
    void authHandler(const QString hostname,
                                const unsigned short port,
                                const QString version,
                                const evutil_socket_t socket,
                                const QUuid uuid,
                                bufferevent* bev);
    void syncHandler(QSharedPointer<unsigned char> syncPacket, size_t len, QUuid senderUuid);
    void nameHandler(QString hname, const QUuid uuid);
    void syncRequestHandlerBev(const bufferevent *bev, const QUuid uuid);

    void newAcceptedConnection(bufferevent* bev);
    void attemptConnection(struct sockaddr_in addr, QUuid uuid);
    void resultOfConnectionAttempt(evutil_socket_t socket, const bool result,
                                   bufferevent* bev, const QUuid uuid);

    void peerQuit(evutil_socket_t s, bufferevent* bev);
    int addMessageToPeer(QString str, QUuid uuid, bool alert, bool);
    void addMessageToAllPeers(QString str, bool alert, bool formatAsMessage);

    void sendMsgAccessor(QByteArray msg, PXMConsts::MESSAGE_TYPE type,
                         QUuid uuid = QUuid());
    void sendUDPAccessor(const char* msg);
    void resultOfTCPSend(const int levelOfSuccess, const QUuid uuid, QString msg,
                         const bool print, const QSharedPointer<Peers::BevWrapper>);

    void setSelfCommsBufferevent(bufferevent* bev);
    void setLocalHostname(QString);
    void setInternalBufferevent(bufferevent* bev);
    void setListenerPorts(unsigned short tcpport, unsigned short udpport);
    void setlibeventBackend(QString str);

    void serverSetupFailure(QString error);
    void multicastIsFunctional();
    void printInfoToDebug();
    void currentThreadInit();
    // void restartServer();
   private slots:
    void beginPeerSync();
    void donePeerSync();
    void requestSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid);
    void discoveryTimerSingleShot();
    void midnightTimerPersistent();
    void discoveryTimerPersistent();
   signals:
    void msgRecieved(QSharedPointer<QString>, QUuid, bool);
    void newAuthedPeer(QUuid, QString);
    void sendMsg(QSharedPointer<Peers::BevWrapper>, QByteArray,
                 PXMConsts::MESSAGE_TYPE, QUuid = QUuid());
    void sendUDP(const char*, unsigned short);
    void sendSyncPacket(QSharedPointer<Peers::BevWrapper>, QSharedPointer<unsigned char>, size_t len,
                       PXMConsts::MESSAGE_TYPE, QUuid = QUuid());
    void connectionStatusChange(QUuid, bool);
    void ipsReceivedFrom(QUuid);
    void warnBox(QString, QString);
};

#endif
