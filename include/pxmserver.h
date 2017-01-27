#ifndef PXMSERVER_H
#define PXMSERVER_H

#include <QThread>
#include <QUuid>
#include <QScopedPointer>
#include <QSharedPointer>

#include <sys/time.h>

#include <event2/util.h>

struct bufferevent;
struct event_base;
class ServerThreadPrivate;

namespace PXMServer
{
const timeval READ_TIMEOUT         = {1, 0};
const timeval READ_TIMEOUT_RESET   = {3600, 0};
const uint8_t PACKET_HEADER_LENGTH = 2;
enum INTERNAL_MSG : uint16_t {
  ADD_DEFAULT_BEV = 0x1111,
  EXIT = 0x2222,
  CONNECT_TO_ADDR = 0x3333,
  TCP_PORT_CHANGE = 0x8888, //Under Construction
  UDP_PORT_CHANGE = 0x9999  //Under Construction
};
class ServerThread : public QThread
{
    Q_OBJECT
    QScopedPointer<ServerThreadPrivate> d_ptr;

   public:
    // Default
    ServerThread(QUuid uuid,
                 QObject* parent        = nullptr,
                 unsigned short tcpPort = 0,
                 unsigned short udpPort = 0,
                 in_addr multicast      = {});
    // Copy
    ServerThread(ServerThread const&) = delete;
    // Copy assignment
    ServerThread& operator=(ServerThread const&) = delete;
    // Move
    ServerThread(ServerThread&& st) noexcept = delete;
    // Move assignment
    ServerThread& operator=(ServerThread&& st) noexcept = delete;
    // Destructor
    ~ServerThread();

    void run() Q_DECL_OVERRIDE;
   signals:
    void messageRecieved(QString, QUuid, bufferevent*, bool);
    void newTCPConnection(bufferevent*);
    void authenticationReceived(QString, unsigned short, QString,
                                evutil_socket_t, QUuid, bufferevent*);
    void peerQuit(evutil_socket_t, bufferevent*);
    void attemptConnection(struct sockaddr_in, QUuid);
    void sendSyncPacket(bufferevent*, QUuid);
    void sendName(bufferevent*, QString, QString);
    void syncPacketIterator(QSharedPointer<unsigned char>, size_t, QUuid);
    void setPeerHostname(QString, QUuid);
    void sendUDP(const char*, unsigned short);
    void setListenerPorts(unsigned short, unsigned short);
    void libeventBackend(QString);
    void setCloseBufferevent(bufferevent*);
    void setSelfCommsBufferevent(bufferevent*);
    void multicastIsFunctional();
    void serverSetupFailure(QString);
    void nameChange(QString, QUuid);
    void resultOfConnectionAttempt(evutil_socket_t, bool, bufferevent*, QUuid);
};
}

#endif  // MESS_SERV_H
