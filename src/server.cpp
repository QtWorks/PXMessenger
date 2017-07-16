#include <server.h>

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __unix__
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#else
#error "include headers for BSD socket implementation"
#endif

#include <QDebug>
#include <QUuid>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/util.h>

#include <openssl/ssl3.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "consts.h"
#include "peers.h"

static_assert(sizeof(uint8_t) == 1, "uint8_t not defined as 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

Q_DECLARE_METATYPE(QSharedPointer<unsigned char>)

using namespace PXMServer;

struct UUIDStruct {
    QUuid uuid;
    ServerThreadPrivate* st;
    bool isSSL = false;
};

class ServerThreadPrivate
{
   public:
    ServerThreadPrivate(ServerThread* q) : q_ptr(q), gotDiscover(false) {}
    ServerThread* q_ptr;
    // Data Members
    QUuid localUUID;
    struct event *eventAccept, *eventDiscover, *eventAcceptSSL;
    QSharedPointer<struct event_base> base;
    SSL_CTX* server_ctx;
    SSL_CTX* client_ctx;
    EVP_PKEY* session_key;
    X509* session_cert;
    in_addr multicastAddress;
    unsigned short tcpPortNumber;
    unsigned short tcpSSLPortNumber;
    unsigned short udpPortNumber;
    bool gotDiscover;
    bool sslReady = true;

    // Functions
    evutil_socket_t newUDPSocket(unsigned short portNumber = 0);
    evutil_socket_t newListenerSocket(unsigned short portNumber);
    unsigned short getPortNumber(evutil_socket_t socket);
    int singleMessageIterator(const bufferevent* bev, const unsigned char* buf, uint16_t len, const QUuid quuid);
    static void internalCommsRead(bufferevent* bev, void*);
    static void accept_new(evutil_socket_t socketfd, short, void* arg);
    static void accept_new_ssl(evutil_socket_t s, short, void* arg);
    static void udpRecieve(evutil_socket_t socketfd, short, void* args);
    static void tcpRead(bufferevent* bev, void* arg);
    static void tcpErr(bufferevent* bev, short error, void* arg);
    static void tcpAuth(bufferevent* bev, void* arg);
    static void connectCB(bufferevent* bev, short error, void* arg);
    void addDefaultBev(bufferevent* bev);
    static void packetLenRecv(bufferevent* bev);
    void acceptSSL(int s, short, void* arg);
    SSL_CTX* ssl_init();
    int newSSLListenerSocket(unsigned short portNumber, SSL_CTX* server_ctx);
    static void waitforHandshake(bufferevent* bev, short event, void* arg);
    EVP_PKEY* createSSLPKey();
    X509* createSSLCert(EVP_PKEY* pkey);
};

ServerThread::ServerThread(QObject* parent,
                           QUuid uuid,
                           in_addr multicast,
                           unsigned short tcpPort,
                           unsigned short udpPort)
    : QThread(parent), d_ptr(new ServerThreadPrivate(this))
{
    d_ptr->tcpPortNumber = tcpPort;

    if (udpPort == 0) {
        d_ptr->udpPortNumber = PXMConsts::DEFAULT_UDP_PORT;
    } else {
        d_ptr->udpPortNumber = udpPort;
    }

    d_ptr->multicastAddress = multicast;
    d_ptr->localUUID        = uuid;
    this->setObjectName("Server Thread");

// Threading might not be needed anymore but left intact for now
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif

    d_ptr->base = QSharedPointer<struct event_base>(event_base_new(), event_base_free);
    qRegisterMetaType<QSharedPointer<unsigned char>>();
}

ServerThread::~ServerThread()
{
    qDebug() << "Shutdown of PXMServer Successful";
}
void ServerThreadPrivate::accept_new_ssl(evutil_socket_t s, short, void* arg)
{
    evutil_socket_t result;

    ServerThreadPrivate* st = static_cast<ServerThreadPrivate*>(arg);

    struct sockaddr_in ss;
    socklen_t addr_size = sizeof(ss);

    result = accept(s, reinterpret_cast<struct sockaddr*>(&ss), &addr_size);
    if (result < 0) {
        qCritical() << "accept: " << QString::fromUtf8(strerror(errno));
    } else {
        struct bufferevent* bev;
        SSL* c_ctx;
        c_ctx = SSL_new(st->server_ctx);
        bev   = bufferevent_openssl_socket_new(st->base.data(), result, c_ctx, BUFFEREVENT_SSL_ACCEPTING,
                                             BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);

        if (!bev)
            qCritical() << "bufferevent_openssl_socket_new failure";
        qCritical() << "SSL ACCEPT";
        // bufferevent_setcb(bev, NULL, NULL, waitforHandshake, arg);
        st->addDefaultBev(bev);
        st->q_ptr->newTCPConnection(bev, false);
    }
    qCritical() << "SSL ACCEPT";
}
void ServerThreadPrivate::accept_new(evutil_socket_t s, short, void* arg)
{
    evutil_socket_t result;

    ServerThreadPrivate* st = static_cast<ServerThreadPrivate*>(arg);

    struct sockaddr_in ss;
    socklen_t addr_size = sizeof(ss);

    result = accept(s, reinterpret_cast<struct sockaddr*>(&ss), &addr_size);
    if (result < 0) {
        qCritical() << "accept: " << QString::fromUtf8(strerror(errno));
    } else {
        struct bufferevent* bev;

        bev = bufferevent_socket_new(st->base.data(), result, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
        if (!bev)
            qCritical() << "bufferevent_openssl_socket_new failure";
        bufferevent_setcb(bev, NULL, NULL, waitforHandshake, arg);
        st->addDefaultBev(bev);
        st->q_ptr->newTCPConnection(bev, false);
    }
}

void ServerThreadPrivate::addDefaultBev(bufferevent* bev)
{
    evutil_make_socket_nonblocking(bufferevent_getfd(bev));

    bufferevent_setcb(bev, ServerThreadPrivate::tcpAuth, NULL, ServerThreadPrivate::tcpErr, this);
    /*
    int s1 = evbuffer_get_length(bufferevent_get_input(bev));
    qDebug() << "Post SSL error evbuf length" << s1;
    int d1 = evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
    qDebug() << "Post SSL error drain" << d1;
    */
    bufferevent_setwatermark(bev, EV_READ, PXMServer::PACKET_HEADER_LEN, PXMServer::PACKET_HEADER_LEN);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}
void ServerThreadPrivate::waitforHandshake(struct bufferevent* bev, short event, void* arg)
{
    ServerThreadPrivate* st = static_cast<ServerThreadPrivate*>(arg);

    qCritical() << QString("%1").arg(event, 0, 16);
    if (event & BEV_EVENT_CONNECTED) {
        st->addDefaultBev(bev);
        st->q_ptr->newTCPConnection(bev, false);
    } else if (event & BEV_EVENT_ERROR) {
        unsigned long sslerr = bufferevent_get_openssl_error(bev);
        if (sslerr == 0) {
            st->q_ptr->peerQuit(bufferevent_getfd(bev), bev);
        } else {
            char* buf = new char[1024];
            ERR_error_string(sslerr, buf);
            qCritical() << "SSL handshake" << buf;
            delete[] buf;
            bufferevent* bev2 = bufferevent_socket_new(st->base.data(), bufferevent_getfd(bev),
                                                       BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
            bufferevent_free(bev);
            int s1 = evbuffer_get_length(bufferevent_get_input(bev2));
            qDebug() << "Post SSL error evbuf length" << s1;
            int d1 = evbuffer_drain(bufferevent_get_input(bev2), UINT16_MAX);
            qDebug() << "Post SSL error drain" << d1;
            st->addDefaultBev(bev2);
            st->q_ptr->newTCPConnection(bev2, false);
        }
    } else {
        st->q_ptr->peerQuit(bufferevent_getfd(bev), bev);
    }
}

EVP_PKEY* ServerThreadPrivate::createSSLPKey()
{
    EVP_PKEY* key = EVP_PKEY_new();
    RSA* rsa      = RSA_new();
    BIGNUM* bne   = BN_new();
    BN_set_word(bne, RSA_F4);
    RSA_generate_key_ex(rsa, 2048, bne, NULL);
    EVP_PKEY_assign_RSA(key, rsa);

    return key;
}

X509* ServerThreadPrivate::createSSLCert(EVP_PKEY* pkey)
{
    X509* cert = X509_new();

    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_getm_notBefore(cert), 0);
    X509_gmtime_adj(X509_getm_notAfter(cert), 60 * 60 * 24 * 365);  // One Year

    X509_set_pubkey(cert, pkey);
    X509_NAME* name = X509_get_subject_name(cert);

    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"US", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"PXMessenger", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"PXMessenger", -1, -1, 0);

    X509_set_issuer_name(cert, name);

    X509_sign(cert, pkey, EVP_sha512());

    return cert;
}

void ServerThreadPrivate::tcpAuth(struct bufferevent* bev, void* arg)
{
    using namespace PXMConsts;
    ServerThreadPrivate* st = static_cast<ServerThreadPrivate*>(arg);
    uint16_t nboBufLen;
    uint16_t bufLen;
    evutil_socket_t socket = bufferevent_getfd(bev);

    // Test for if we have only gotten 1 byte of data so far, need 2
    // int t1 = evbuffer_get_length(bufferevent_get_input(bev));
    if (evbuffer_get_length(bufferevent_get_input(bev)) == PACKET_HEADER_LEN) {
        evbuffer_copyout(bufferevent_get_input(bev), &nboBufLen, PACKET_HEADER_LEN);
        bufLen = ntohs(nboBufLen);
        if (bufLen == 0 || bufLen > MAX_AUTH_PACKET_LEN) {
            qWarning().noquote() << "Bad buffer length, disconnecting - " + QString::number(bufLen);
            bufferevent_disable(bev, EV_READ | EV_WRITE);
            st->q_ptr->peerQuit(socket, bev);
            return;
        }
        bufferevent_setwatermark(bev, EV_READ, bufLen + PACKET_HEADER_LEN, bufLen + PACKET_HEADER_LEN);
        return;
    }
    // Read How many bytes are coming from the first 2 bytes
    bufferevent_read(bev, &nboBufLen, PACKET_HEADER_LEN);

    bufLen = ntohs(nboBufLen);

    // Read the number of bytes that are supposed to be here
    unsigned char bufUUID[NetCompression::PACKED_UUID_LENGTH];
    if (bufferevent_read(bev, bufUUID, NetCompression::PACKED_UUID_LENGTH) < NetCompression::PACKED_UUID_LENGTH) {
        qWarning() << "Bad Auth packet length, closing socket...";
        bufferevent_disable(bev, EV_READ | EV_WRITE);
        st->q_ptr->peerQuit(socket, bev);
        return;
    }
    // Get the uuid which should be immediately after the length segment
    QUuid quuid = QUuid();
    bufLen -= NetCompression::unpackUUID(bufUUID, quuid);
    if (quuid.isNull()) {
        qWarning() << "Bad Auth packet UUID, closing socket...";
        bufferevent_disable(bev, EV_READ | EV_WRITE);
        st->q_ptr->peerQuit(socket, bev);
        return;
    }

    // Get the rest of the packet
    unsigned char buf[bufLen + 1];
    bufferevent_read(bev, buf, bufLen);
    buf[bufLen] = 0;

    // Process the full auth packet's information
    // Auth packet format "Hostname:::12345:::001.001.001"
    bufferevent_disable(bev, EV_READ | EV_WRITE);
    MESSAGE_TYPE* type = reinterpret_cast<MESSAGE_TYPE*>(&buf[0]);
    if (*type == MSG_AUTH) {
        bufLen -= sizeof(MESSAGE_TYPE);
        QStringList hpsplit = (QString::fromUtf8((char*)&buf[sizeof(MESSAGE_TYPE)], bufLen)).split(AUTH_SEPERATOR);

        st->q_ptr->authHandler(hpsplit, socket, quuid, bev);
        bufferevent_setwatermark(bev, EV_READ, PACKET_HEADER_LEN, PACKET_HEADER_LEN);
        bufferevent_setcb(bev, ServerThreadPrivate::tcpRead, NULL, ServerThreadPrivate::tcpErr, st);
    } else {
        qWarning() << "Non-Auth packet, closing socket...";
        st->q_ptr->peerQuit(socket, bev);
    }
}
void ServerThreadPrivate::packetLenRecv(struct bufferevent* bev)
{
    uint16_t nboBufLen;
    uint16_t bufLen;
    evbuffer* input = bufferevent_get_input(bev);

    qDebug().noquote() << "Recieved bufferlength value";
    evbuffer_copyout(input, &nboBufLen, PACKET_HEADER_LEN);
    bufLen = ntohs(nboBufLen);
    if (bufLen == 0) {
        qWarning().noquote() << "Bad buffer length, draining...";
        evbuffer_drain(input, UINT16_MAX);
        return;
    }
    bufferevent_setwatermark(bev, EV_READ, bufLen + PACKET_HEADER_LEN, bufLen + PACKET_HEADER_LEN);
    bufferevent_set_timeouts(bev, &READ_TIMEOUT, NULL);
    qDebug().noquote() << "Setting watermark to" << QString::number(bufLen) << "bytes";
    qDebug().noquote() << "Setting timeout to"
                       << QString::asprintf("%ld.%06ld", READ_TIMEOUT.tv_sec, READ_TIMEOUT.tv_usec) << "seconds";
}

void ServerThreadPrivate::tcpRead(struct bufferevent* bev, void* arg)
{
    ServerThreadPrivate* st = static_cast<ServerThreadPrivate*>(arg);
    evbuffer* input         = bufferevent_get_input(bev);

    if (evbuffer_get_length(input) == 1) {
        qDebug().noquote() << "Setting timeout, 1 byte recieved";
        bufferevent_set_timeouts(bev, &READ_TIMEOUT, NULL);
    } else if (evbuffer_get_length(input) == PACKET_HEADER_LEN) {
        packetLenRecv(bev);
    } else {
        uint16_t nboBufLen;
        uint16_t bufLen;
        bufferevent_setwatermark(bev, EV_READ, PACKET_HEADER_LEN, PACKET_HEADER_LEN);

        bufferevent_read(bev, &nboBufLen, PACKET_HEADER_LEN);

        // convert from NBO to host endianness
        bufLen = ntohs(nboBufLen);
        // check if packet is too small to contain a UUID
        if (bufLen <= NetCompression::PACKED_UUID_LENGTH) {
            evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
            return;
        }

        // Extract and Unpack uuid from the packet
        unsigned char rawUUID[NetCompression::PACKED_UUID_LENGTH];
        bufferevent_read(bev, rawUUID, NetCompression::PACKED_UUID_LENGTH);
        QUuid uuid = QUuid();
        bufLen -= NetCompression::unpackUUID(rawUUID, uuid);

        // Check if uuid is null
        if (uuid.isNull()) {
            evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
            return;
        }

        // Change length of msg with respect to uuid length
        unsigned char buf[bufLen + 1];

        // Read rest of message
        bufferevent_read(bev, buf, bufLen);

        st->singleMessageIterator(bev, buf, bufLen, uuid);

        // reset timeout -- this is a bug with libevent
        // timeout is set to 1 day, a value of null in the read
        // parameter does nothing.  Timeing out should not cause a
        // problem
        bufferevent_set_timeouts(bev, &READ_TIMEOUT_RESET, NULL);

        // delete[] buf;
    }
}

void ServerThreadPrivate::tcpErr(struct bufferevent* bev, short error, void* arg)
{
    ServerThreadPrivate* st = static_cast<ServerThreadPrivate*>(arg);
    evutil_socket_t i       = bufferevent_getfd(bev);
    // EOF should be for close, ERROR could be for closed if we miss an ACK
    // somewhere. TIMEOUT should be happening only if we get a packet of a
    // size that is smaller than what the sender has told us it will be
    if (error & BEV_EVENT_EOF) {
        qDebug() << "BEV EOF";
        bufferevent_disable(bev, EV_READ | EV_WRITE);
        st->q_ptr->peerQuit(i, bev);
    } else if (error & BEV_EVENT_ERROR) {
        qDebug() << "BEV ERROR";
        bufferevent_disable(bev, EV_READ | EV_WRITE);
        st->q_ptr->peerQuit(i, bev);
    } else if (error & BEV_EVENT_TIMEOUT) {
        qDebug() << "BEV TIMEOUT";
        // Reset watermark for accepting a length arguement
        bufferevent_setwatermark(bev, EV_READ, PACKET_HEADER_LEN, PACKET_HEADER_LEN);
        // Reset timeout to 1 day, bug in libevent; fixed in 2.1 -- maybe
        bufferevent_set_timeouts(bev, &READ_TIMEOUT_RESET, NULL);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        // Drain anything left in buffer
        evbuffer* input = bufferevent_get_input(bev);
        size_t len      = evbuffer_get_length(input);
        if (len > 0) {
            qDebug() << "Length:" << len;
            qDebug() << "Draining...";
            evbuffer_drain(input, UINT16_MAX);
            len = evbuffer_get_length(input);
            qDebug() << "Length: " << len;
        }
    }
}
int ServerThreadPrivate::singleMessageIterator(const bufferevent* bev,
                                               const unsigned char* buf,
                                               uint16_t bufLen,
                                               const QUuid quuid)
{
    using namespace PXMConsts;
    // Should always have something in the buffer
    if (bufLen == 0) {
        qCritical() << "Blank message! -- Not Good!";
        return -1;
    }
    MESSAGE_TYPE type = *reinterpret_cast<const MESSAGE_TYPE*>(&buf[0]);
    type              = static_cast<MESSAGE_TYPE>(ntohl(type));
    buf               = &buf[sizeof(MESSAGE_TYPE)];
    bufLen -= sizeof(MESSAGE_TYPE);
    QSharedPointer<unsigned char> bufCopy = QSharedPointer<unsigned char>(new unsigned char[bufLen]);
    memcpy(bufCopy.data(), buf, bufLen);

    emit q_ptr->packetHandler(bufCopy, bufLen, type, quuid, bev);

    return 0;
}
void ServerThreadPrivate::udpRecieve(evutil_socket_t socketfd, short int, void* args)
{
    ServerThreadPrivate* st = static_cast<ServerThreadPrivate*>(args);
    struct sockaddr_in si_other;
    socklen_t si_other_len = sizeof(struct sockaddr);

    // Max udp message size, should be much smaller for our program
    char buf[200] = {};

    // Non libevent read from this socket, maybe change in future
    recvfrom(socketfd, buf, sizeof(buf) - 1, 0, reinterpret_cast<struct sockaddr*>(&si_other), &si_other_len);

    // Discovery packet handler
    if (strncmp(&buf[0], "/discover", 9) == 0) {
        qDebug() << "Discovery Packet:" << buf;

        // This confirms we got a multicast packet, first one should be
        // our own.
        if (!st->gotDiscover) {
            st->gotDiscover = true;
            st->q_ptr->multicastIsFunctional();
        }

        // Socket to reply to the discover packet, maybe this should be
        // done seperate from this callback
        evutil_socket_t replySocket;
        if ((replySocket = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0) {
            qCritical().noquote() << "socket: " + QString::fromUtf8(strerror(errno));
            qCritical().noquote() << "Reply to /discover packet not sent";
            return;
        }

        // Set reply destination to the multicast port number
        si_other.sin_port = htons(st->udpPortNumber);

        // Format reply message, ct_strlen works around a compiler warning when
        // making this a const.
        constexpr size_t len = sizeof(uint16_t) + NetCompression::PACKED_UUID_LENGTH + PXMConsts::ct_strlen("/name:");

        char name[len + 1];

        strcpy(name, "/name:");

        uint16_t port = htons(st->tcpPortNumber);
        memcpy(&name[strlen("/name:")], &(port), sizeof(port));
        NetCompression::packUUID((unsigned char*)&name[strlen("/name:") + sizeof(port)], st->localUUID);

        name[len] = 0;

        // Send reply message twice to ensure one of them gets there
        for (int k = 0; k < 2; k++) {
            if (sendto(replySocket, name, len, 0, reinterpret_cast<struct sockaddr*>(&si_other), si_other_len) != len)
                qCritical().noquote() << "sendto: " + QString::fromUtf8(strerror(errno));
        }

        // close reply socket
        evutil_closesocket(replySocket);

    } else if ((strncmp(&buf[0], "/name:", 6)) == 0) {
        // Get port number of their TCP listener
        memcpy(&si_other.sin_port, &buf[6], sizeof(uint16_t));
        // Get their uuid
        QUuid uuid;
        NetCompression::unpackUUID(reinterpret_cast<unsigned char*>(&buf[8]), uuid);
        qDebug() << "Name Packet:" << inet_ntoa(si_other.sin_addr) << ":" << ntohs(si_other.sin_port)
                 << "with id:" << uuid.toString();
        /* Send this info along to peerworker */
        st->q_ptr->attemptConnection(si_other, uuid, false);
    } else {
        qWarning() << "Bad udp packet!";
    }

    return;
}
unsigned short ServerThreadPrivate::getPortNumber(evutil_socket_t socket)
{
    struct sockaddr_in needPortNumber;
    memset(&needPortNumber, 0, sizeof(needPortNumber));
    socklen_t needPortNumberLen = sizeof(needPortNumber);
    if (getsockname(socket, reinterpret_cast<struct sockaddr*>(&needPortNumber), &needPortNumberLen) != 0) {
        qDebug().noquote() << "getsockname: " + QString::fromUtf8(strerror(errno));
        return 0;
    }
    return ntohs(needPortNumber.sin_port);
}
evutil_socket_t ServerThreadPrivate::newUDPSocket(unsigned short portNumber)
{
    struct sockaddr_in si_me;
    ip_mreq multicastGroup;

    evutil_socket_t socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    multicastGroup.imr_multiaddr        = multicastAddress;
    multicastGroup.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(socketUDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char*>(&multicastGroup),
                   sizeof(multicastGroup)) < 0) {
        qCritical().noquote() << "setsockopt: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketUDP);
        return -1;
    }

    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port   = htons(portNumber);
    // si_me.sin_addr.s_addr = inet_addr(MULTICAST_ADDRESS);
    si_me.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, "true", sizeof(int)) < 0) {
        qCritical().noquote() << "setsockopt: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketUDP);
        return -1;
    }

    evutil_make_socket_nonblocking(socketUDP);

    if (bind(socketUDP, reinterpret_cast<struct sockaddr*>(&si_me), sizeof(struct sockaddr))) {
        qCritical().noquote() << "bind: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketUDP);
        return -1;
    }

    return socketUDP;
}

SSL_CTX* ServerThreadPrivate::ssl_init()
{
    SSL_CTX* server_ctx = nullptr;

    SSL_library_init();
    ERR_load_ERR_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    RAND_poll();

    server_ctx = SSL_CTX_new(SSLv23_server_method());

    session_key  = createSSLPKey();
    session_cert = createSSLCert(session_key);

    if (!SSL_CTX_use_certificate(server_ctx, session_cert)) {
        qCritical() << "SSL cert Error";
    }
    if (!SSL_CTX_use_PrivateKey(server_ctx, session_key)) {
        qCritical() << "SSL key Error";
    }

    SSL_CTX_set_options(server_ctx, SSL_OP_NO_SSLv2);

    return server_ctx;
}

evutil_socket_t ServerThreadPrivate::newListenerSocket(unsigned short portNumber)
{
    struct addrinfo hints, *res;
    QVector<evutil_socket_t> tcpSockets;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    char tcpPortChar[6] = {};
    sprintf(tcpPortChar, "%d", portNumber);

    if (getaddrinfo(NULL, tcpPortChar, &hints, &res) < 0) {
        qCritical().noquote() << "getaddrinfo: " + QString::fromUtf8(strerror(errno));
        return -1;
    }

    // There could be multiple sockets here, i need to see this actually
    // happen once to write code
    // to account for it.  The for loop runs but we still only use the first
    // socket that comes out of here
    for (addrinfo* p = res; p; p = p->ai_next) {
        evutil_socket_t socketTCP;
        if ((socketTCP = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            qCritical().noquote() << "socket: " + QString::fromUtf8(strerror(errno));
            continue;
        }
        if (setsockopt(socketTCP, SOL_SOCKET, SO_REUSEADDR, "true", sizeof(int)) < 0) {
            qCritical().noquote() << "setsockopt: " + QString::fromUtf8(strerror(errno));
            continue;
        }

        evutil_make_socket_nonblocking(socketTCP);

        if (bind(socketTCP, res->ai_addr, res->ai_addrlen) < 0) {
            qCritical().noquote() << "bind: " + QString::fromUtf8(strerror(errno));
            continue;
        }
        if (listen(socketTCP, SOMAXCONN) < 0) {
            qCritical().noquote() << "listen: " + QString::fromUtf8(strerror(errno));
            continue;
        }
        tcpSockets.append(socketTCP);
        qInfo().noquote() << "Port number for TCP/IP Listener: " + QString::number(getPortNumber(socketTCP));
    }
    qDebug().noquote() << "Number of tcp sockets:" << tcpSockets.length();

    freeaddrinfo(res);

    return tcpSockets.first();
}
void ServerThreadPrivate::internalCommsRead(bufferevent* bev, void* args)
{
    // Other than the exit message this is under heavy construction and not
    // in use yet
    unsigned char readBev[INTERNAL_MSG_LENGTH] = {};
    ServerThreadPrivate* st                    = static_cast<ServerThreadPrivate*>(args);
    bufferevent_read(bev, readBev, INTERNAL_MSG_LENGTH);
    INTERNAL_MSG* type = reinterpret_cast<INTERNAL_MSG*>(&readBev[0]);
    int index          = sizeof(INTERNAL_MSG);
    switch (*type) {
        case ADD_DEFAULT_BEV: {
            // copy out a pointer address that we are adding with a default
            // setup
            struct bufferevent** newBev = reinterpret_cast<struct bufferevent**>(&readBev[sizeof(INTERNAL_MSG)]);
            st->addDefaultBev(*newBev);
        } break;
        case EXIT:
            event_base_loopexit(st->base.data(), NULL);
            break;
        case CONNECT_TO_ADDR: {
            struct sockaddr_in addr;
            QUuid uuid = QUuid();

            index += NetCompression::unpackSockaddr_in(&readBev[index], addr);
            index += NetCompression::unpackUUID(&readBev[index], uuid);
            addr.sin_family = AF_INET;

            evutil_socket_t socketfd = socket(AF_INET, SOCK_STREAM, 0);
            evutil_make_socket_nonblocking(socketfd);
            struct bufferevent* bev =
                bufferevent_socket_new(st->base.data(), socketfd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
            if (!bev)
                qCritical() << "Could not create bufferevent socket";

            UUIDStruct* uuidStruct = new UUIDStruct{uuid, st, false};
            bufferevent_setcb(bev, NULL, NULL, ServerThreadPrivate::connectCB, uuidStruct);
            timeval timeout = {5, 0};
            bufferevent_set_timeouts(bev, &timeout, &timeout);
            bufferevent_socket_connect(bev, reinterpret_cast<struct sockaddr*>(&addr), sizeof(sockaddr_in));
        } break;
        case SSL_CONNECT_TO_ADDR: {
            struct sockaddr_in addr;
            QUuid uuid = QUuid();

            index += NetCompression::unpackSockaddr_in(&readBev[index], addr);
            index += NetCompression::unpackUUID(&readBev[index], uuid);
            addr.sin_family = AF_INET;

            evutil_socket_t socketfd = socket(AF_INET, SOCK_STREAM, 0);
            evutil_make_socket_nonblocking(socketfd);
            SSL* client_ssl = SSL_new(st->client_ctx);
            struct bufferevent* bev =
                bufferevent_openssl_socket_new(st->base.data(), socketfd, client_ssl, BUFFEREVENT_SSL_CONNECTING,
                                               BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
            if (!bev)
                qCritical() << "Could not create bufferevent socket";

            UUIDStruct* uuidStruct = new UUIDStruct{uuid, st, true};
            bufferevent_setcb(bev, NULL, NULL, ServerThreadPrivate::connectCB, uuidStruct);
            timeval timeout = {5, 0};
            bufferevent_set_timeouts(bev, &timeout, &timeout);
            bufferevent_socket_connect(bev, reinterpret_cast<struct sockaddr*>(&addr), sizeof(sockaddr_in));
        } break; /*
         case TCP_PORT_CHANGE:
                 if (st->eventAccept) {
                 evutil_closesocket(event_get_fd(st->eventAccept));
                 event_free(st->eventAccept);
                 st->eventAccept = 0;
                 }
                 try {
                 unsigned short newPortNumber =
                *(reinterpret_cast<unsigned
                short*>(&readBev[sizeof(INTERNAL_MSG)]));
                 evutil_socket_t s_listen     =
                st->newListenerSocket(newPortNumber);
                 st->eventAccept =
                     event_new(st->q_ptr->base.data(), s_listen, EV_READ
                | EV_PERSIST, st->accept_new,
                st->q_ptr);
                 if (!st->eventAccept) {
                     throw("FATAL:event_new returned NULL");
                 }
                 } catch (const char* errorMsg) {
                 qCritical() << errorMsg;
                 st->q_ptr->serverSetupFailure(errorMsg);
                 return;
                 } catch (QString errorMsg) {
                 qCritical() << errorMsg;
                 st->q_ptr->serverSetupFailure(errorMsg);
                 return;
                 }
             break;
                 */
        /*
        case UDP_PORT_CHANGE:
                if (st->eventDiscover) {
                evutil_closesocket(event_get_fd(st->eventDiscover));
                event_free(st->eventDiscover);
                st->eventDiscover = 0;
                }
                try {
                unsigned short newPortNumber =
               *(reinterpret_cast<unsigned
               short*>(&readBev[sizeof(INTERNAL_MSG)]));
                evutil_socket_t s_udp        =
               st->newUDPSocket(newPortNumber);
                st->eventDiscover =
                    event_new(st->q_ptr->base.data(), s_udp, EV_READ |
               EV_PERSIST, st->accept_new, st->q_ptr);
                if (!st->eventDiscover) {
                    throw("FATAL:event_new returned NULL");
                }
                } catch (const char* errorMsg) {
                qCritical() << errorMsg;
                st->q_ptr->serverSetupFailure(errorMsg);
                return;
                } catch (QString errorMsg) {
                qCritical() << errorMsg;
                st->q_ptr->serverSetupFailure(errorMsg);
                return;
                }
            break;
                */
        default:
            bufferevent_flush(bev, EV_READ, BEV_FLUSH);
            qDebug() << "Bad Internal comms";
            break;
    }
}
void ServerThreadPrivate::connectCB(struct bufferevent* bev, short event, void* arg)
{
    UUIDStruct* st = static_cast<UUIDStruct*>(arg);
    if (event & BEV_EVENT_CONNECTED) {
        qDebug() << "succ connect";
        st->st->q_ptr->resultOfConnectionAttempt(bufferevent_getfd(bev), true, bev, st->uuid, st->isSSL);
    } else {
        qDebug() << "bad connect" << event;
        unsigned long sslerr = bufferevent_get_openssl_error(bev);
        QString errhex       = QString("%1").arg(sslerr, 0, 16);
        char sslerr_c[1024];
        ERR_error_string(sslerr, sslerr_c);
        qCritical() << errhex << sslerr_c;
        st->st->q_ptr->resultOfConnectionAttempt(bufferevent_getfd(bev), false, bev, st->uuid, false);
    }
    delete st;
}

void ServerThread::run()
{
    int failureCodes;

    // error if pointer is null
    if (!d_ptr->base) {
        QString errorMsg = "FATAL:event_base_new returned NULL";
        qCritical() << errorMsg;
        serverSetupFailure(errorMsg);
        return;
    }

    d_ptr->server_ctx = d_ptr->ssl_init();
    if (!d_ptr->server_ctx) {
        d_ptr->sslReady = false;
        qWarning() << "SSL init has failed";
    }

    d_ptr->client_ctx = SSL_CTX_new(SSLv23_client_method());

    // Communicate backend to peerworker
    qDebug().noquote() << "Using " + QString::fromUtf8(event_base_get_method(d_ptr->base.data())) +
                              " as the libevent backend";
    emit libeventBackend(QString::fromUtf8(event_base_get_method(d_ptr->base.data())),
                         QString::fromLatin1(event_get_version()));

    // Pair for self communication
    struct bufferevent* selfCommsPair[2];
    bufferevent_pair_new(d_ptr->base.data(), BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS, selfCommsPair);
    bufferevent_setcb(selfCommsPair[0], ServerThreadPrivate::tcpRead, NULL, ServerThreadPrivate::tcpErr, d_ptr.data());
    bufferevent_setwatermark(selfCommsPair[0], EV_READ, PACKET_HEADER_LEN, PACKET_HEADER_LEN);
    bufferevent_enable(selfCommsPair[0], EV_READ);
    bufferevent_enable(selfCommsPair[1], EV_WRITE);

    emit setSelfCommsBufferevent(selfCommsPair[1]);

    // TCP listener socket setup
    evutil_socket_t s_listen, ssl_listen;
    if (d_ptr->sslReady) {
        // ssl_listen = d_ptr->newListenerSocket(d_ptr->tcpSSLPortNumber);
        ssl_listen = d_ptr->newListenerSocket(0);
    }
    s_listen = d_ptr->newListenerSocket(d_ptr->tcpPortNumber);
    if (s_listen < 0) {
        QString errorMsg = "FATAL:TCP Listening socket setup has failed";
        qCritical() << errorMsg;
        serverSetupFailure(errorMsg);
        return;
    }

    // UDP multicast socket setup
    evutil_socket_t s_discover;
    s_discover = d_ptr->newUDPSocket(d_ptr->udpPortNumber);
    if (s_discover < 0) {
        QString errorMsg = "FATAL:UDP Multicast socket setup has failed";
        qCritical() << errorMsg;
        serverSetupFailure(errorMsg);
        return;
    }

    // Tell peerworker what our socket numbers are
    d_ptr->tcpPortNumber = d_ptr->getPortNumber(s_listen);

    d_ptr->udpPortNumber = d_ptr->getPortNumber(s_discover);

    d_ptr->tcpSSLPortNumber = d_ptr->getPortNumber(ssl_listen);

    emit setListenerPorts(d_ptr->tcpPortNumber, d_ptr->udpPortNumber, d_ptr->tcpSSLPortNumber);

    qInfo().noquote() << "Port number for Multicast: " + QString::number(d_ptr->udpPortNumber);

    try {
        d_ptr->eventDiscover =
            event_new(d_ptr->base.data(), s_discover, EV_READ | EV_PERSIST, d_ptr->udpRecieve, d_ptr.data());
        if (!d_ptr->eventDiscover) {
            throw("FATAL:event_new returned NULL");
        }

        failureCodes = event_add(d_ptr->eventDiscover, NULL);
        if (failureCodes < 0) {
            throw("FATAL:event_add returned -1");
        }

        d_ptr->eventAccept =
            event_new(d_ptr->base.data(), s_listen, EV_READ | EV_PERSIST, d_ptr->accept_new, d_ptr.data());
        d_ptr->eventAcceptSSL =
            event_new(d_ptr->base.data(), ssl_listen, EV_READ | EV_PERSIST, d_ptr->accept_new_ssl, d_ptr.data());
        if (!d_ptr->eventAccept) {
            throw("FATAL:event_new returned NULL");
        }

        failureCodes = event_add(d_ptr->eventAccept, NULL);
        if (failureCodes < 0) {
            throw("FATAL:event_add returned -1");
        }
        failureCodes = event_add(d_ptr->eventAcceptSSL, NULL);
        if (failureCodes < 0) {
            throw("FATAL:event_add returned -1");
        }
    } catch (const char* errorMsg) {
        qCritical() << errorMsg;
        serverSetupFailure(errorMsg);
        return;
    } catch (QString errorMsg) {
        qCritical() << errorMsg;
        serverSetupFailure(errorMsg);
        return;
    }

    // send our discover packet to find other computers
    emit sendUDP("/discover", d_ptr->udpPortNumber);

    // Pair to shutdown server
    struct bufferevent* internalCommsPair[2];
    bufferevent_pair_new(d_ptr->base.data(), BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS, internalCommsPair);
    bufferevent_setcb(internalCommsPair[0], ServerThreadPrivate::internalCommsRead, NULL, NULL, d_ptr.data());
    bufferevent_setwatermark(internalCommsPair[0], EV_READ, INTERNAL_MSG_LENGTH, INTERNAL_MSG_LENGTH);
    bufferevent_enable(internalCommsPair[0], EV_READ);
    bufferevent_enable(internalCommsPair[1], EV_WRITE);

    emit setInternalBufferevent(internalCommsPair[1]);

    // Start event loop, this is shutdown using the internalCommsPair by
    // sending an EXIT message
    failureCodes = event_base_dispatch(d_ptr->base.data());
    if (failureCodes < 0) {
        qWarning() << "event_base_dispatch shutdown with error";
    }

    // Free libevent data structures before exiting the thread
    qDebug() << "Freeing events...";
    event_free(d_ptr->eventAccept);
    event_free(d_ptr->eventAcceptSSL);
    event_free(d_ptr->eventDiscover);

    bufferevent_free(internalCommsPair[1]);
    bufferevent_free(internalCommsPair[0]);

    bufferevent_free(selfCommsPair[1]);
    bufferevent_free(selfCommsPair[0]);

    EVP_PKEY_free(d_ptr->session_key);
    X509_free(d_ptr->session_cert);

    qDebug() << "Events free, returning from PXMServer::run()";
}