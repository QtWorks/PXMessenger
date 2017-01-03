#ifndef PXMPEERWORKER_H
#define PXMPEERWORKER_H

#include <QString>
#include <QDebug>
#include <QMutex>
#include <QUuid>
#include <QThread>
#include <QTimer>
#include <QStringBuilder>
#include <QBuffer>
#include <QApplication>
#include <QDateTime>
#include <QRegularExpression>

#include <sys/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <event2/bufferevent.h>

#include "pxmserver.h"
#include "pxmsync.h"
#include "pxmdefinitions.h"
#include "pxmclient.h"
#include "timedvector.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include <QSharedPointer>
#endif

class PXMPeerWorker : public QObject
{
    Q_OBJECT
public:
    explicit PXMPeerWorker(QObject *parent,
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
    PXMPeerWorker(PXMPeerWorker&&) noexcept = delete;
    const int SYNC_TIMEOUT_MSECS = 2000;
    const int SYNC_TIMER = 900000;
private:
    QHash<QUuid,Peers::PeerData>peerDetailsHash;
    QString localHostname;
    QUuid localUUID;
    QString multicastAddress;
    unsigned short serverTCPPort;
    unsigned short serverUDPPort;
    QString libeventBackend;
    QUuid globalUUID;
    QTimer *syncTimer;
    QTimer *nextSyncTimer;
    QTimer *discoveryTimer;
    QTimer *midnightTimer;
    bool areWeSyncing;
    bool multicastIsFunctioning;
    PXMSync *syncer;
    QVector<Peers::BevWrapper*> bwShortLife;
    PXMServer::ServerThread *messServer;
    PXMClient *messClient;
    bufferevent *closeBev;
    QVector<QSharedPointer<Peers::BevWrapper>> extraBufferevents;
    TimedVector<QUuid> *syncablePeers;


    void sendAuthPacket(QSharedPointer<Peers::BevWrapper> bw);
    void startServer();
    void connectClient();
    int formatMessage(QString &str, QUuid uuid, QString color);
public slots:
    void setListenerPorts(unsigned short tcpport, unsigned short udpport);
    void syncPacketIterator(char *ipHeapArray, size_t len, QUuid senderUuid);
    void attemptConnection(sockaddr_in addr, QUuid uuid);
    void authenticationReceived(QString hname, unsigned short port,
                                evutil_socket_t s, QUuid uuid,
                                bufferevent *bev);
    void newTcpConnection(bufferevent *bev);
    void peerQuit(evutil_socket_t s, bufferevent *bev);
    void peerNameChange(QString hname, QUuid uuid);
    void sendSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid);
    void sendSyncPacketBev(bufferevent *bev, QUuid uuid);
    void resultOfConnectionAttempt(evutil_socket_t socket, bool result,
                                   bufferevent *bev);
    void resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg,
                         bool print, QSharedPointer<Peers::BevWrapper> bw);
    void currentThreadInit();
    int addMessageToPeer(QString str, QUuid uuid, bool alert,
                         bool formatAsMessage);
    void printInfoToDebug();
    void setlibeventBackend(QString str);
    int recieveServerMessage(QString str, QUuid uuid, bufferevent *bev,
                             bool global);
    void addMessageToAllPeers(QString str, bool alert, bool formatAsMessage);
    void printFullHistory(QUuid uuid);
    void sendMsgAccessor(QByteArray msg, PXMConsts::MESSAGE_TYPE type, QUuid uuid = QUuid());
    void setSelfCommsBufferevent(bufferevent *bev);
    void discoveryTimerPersistent();
    void multicastIsFunctional();
    void serverSetupFailure();
    void setLocalHostname(QString);
    void sendUDPAccessor(const char* msg);
private slots:
    void beginSync();
    void doneSync();
    void requestSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid);
    void discoveryTimerSingleShot();
    void midnightTimerPersistent();
    void setCloseBufferevent(bufferevent *bev);
signals:
    void printToTextBrowser(QString, QUuid, bool);
    void updateListWidget(QUuid, QString);
    void sendMsg(QSharedPointer<Peers::BevWrapper>, QByteArray, PXMConsts::MESSAGE_TYPE,
                 QUuid = QUuid());
    void sendUDP(const char*, unsigned short);
    void sendIpsPacket(QSharedPointer<Peers::BevWrapper>, char*, size_t len,
                       PXMConsts::MESSAGE_TYPE, QUuid = QUuid());
    void connectToPeer(evutil_socket_t, sockaddr_in, QSharedPointer<Peers::BevWrapper>);
    void updateMessServFDS(evutil_socket_t);
    void setItalicsOnItem(QUuid, bool);
    void ipsReceivedFrom(QUuid);
    void warnBox(QString, QString);
    void fullHistory(QLinkedList<QString*>, QUuid);
};

#endif
