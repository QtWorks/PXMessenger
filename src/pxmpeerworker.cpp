#include <pxmpeerworker.h>

PXMPeerWorker::PXMPeerWorker(QObject *parent, QString hostname, QString uuid, PXMServer *server) : QObject(parent)
{
    localHostname = hostname;
    localUUID = uuid;
    realServer = server;
    syncer = new PXMSync(this);
    QObject::connect(syncer, &PXMSync::requestIps, this, &PXMPeerWorker::requestIps);
    QObject::connect(syncer, &PXMSync::syncComplete, this, &PXMPeerWorker::doneSync);
    srand (time(NULL));
    syncTimer = new QTimer(this);
    //syncTimer->setInterval((rand() % 300000) + 900000);
    syncTimer->setInterval(10000);
    QObject::connect(syncTimer, SIGNAL(timeout()), this, SLOT(beginSync()));
    syncTimer->start();
    nextSyncTimer = new QTimer(this);
    nextSyncTimer->setInterval(1000);
    QObject::connect(nextSyncTimer, SIGNAL(timeout()), syncer, SLOT(syncNext()));
}
PXMPeerWorker::~PXMPeerWorker()
{
    syncTimer->stop();
    //if(syncer->isRunning())
    //{
     //   syncer->requestInterruption();
      //  syncer->quit();
       // syncer->wait(5000);
    //}
    //syncer->deleteLater();
    for(auto &itr : peerDetailsHash)
    {
        qDeleteAll(itr.messages);
    }
    qDebug() << "Shutdown of PXMPeerWorker Successful";
}

void PXMPeerWorker::setLocalHostName(QString name)
{
    localHostname = name;
}
void PXMPeerWorker::setListenerPort(unsigned short port)
{
    ourListenerPort = QString::number(port);
}
void PXMPeerWorker::doneSync()
{
    areWeSyncing = false;
    nextSyncTimer->stop();
    qDebug() << "Finished Syncing peers";
}

void PXMPeerWorker::beginSync()
{
    /*if(syncer->isRunning())
        return;
    syncer->setHash(peerDetailsHash);
    qDebug() << "Beginning Sync of connected peers";
    areWeSyncing = true;
    syncer->start();
    */
    if(areWeSyncing)
        return;

    qDebug() << "Beginning Sync of connected peers";
    areWeSyncing = true;
    syncer->setsyncHash(&peerDetailsHash);
    syncer->setIteratorToStart();
    syncer->syncNext();
    nextSyncTimer->start();
}
void PXMPeerWorker::hostnameCheck(char *ipHeapArray, size_t len, QUuid senderUuid)
{
    if(senderUuid != waitingOnIpsFrom)
    {
        delete [] ipHeapArray;
        return;
    }
    qDebug() << "Recieved connection list from" << peerDetailsHash[senderUuid].socketDescriptor;

    size_t index = 0;
    while(index < len)
    {
        sockaddr_in addr;
        QUuid uuid;
        addr.sin_family = AF_INET;
        memcpy(&(addr.sin_addr.s_addr), ipHeapArray+index, sizeof(uint32_t));
        index += sizeof(uint32_t);
        memcpy(&(addr.sin_port), ipHeapArray+index, sizeof(uint16_t));
        index += sizeof(uint16_t);
        memcpy(&(uuid.data1), ipHeapArray+index, sizeof(uint32_t));
        index += sizeof(uint32_t);
        uuid.data1 = ntohl(uuid.data1);
        memcpy(&(uuid.data2), ipHeapArray+index, sizeof(uint16_t));
        index += sizeof(uint16_t);
        uuid.data2 = ntohs(uuid.data2);
        memcpy(&(uuid.data3), ipHeapArray+index, sizeof(uint16_t));
        index += sizeof(uint16_t);
        uuid.data3 = ntohs(uuid.data3);
        memcpy(&(uuid.data4), ipHeapArray+index, 8);
        index += 8;
        qDebug() << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ":" << uuid.toString();
        attemptConnection(addr, uuid);
    }

    waitingOnIpsFrom = QUuid();
    if(areWeSyncing)
    {
        nextSyncTimer->stop();
        syncer->syncNext();
        nextSyncTimer->start(1000);
    }

    delete [] ipHeapArray;
}
void PXMPeerWorker::newTcpConnection(evutil_socket_t s)
{
    this->sendIdentityMsg(s);
}
void PXMPeerWorker::sendIdentityMsg(evutil_socket_t s)
{
    emit sendMsg(s, (localHostname % QStringLiteral("::::") % ourListenerPort).toUtf8(), "/uuid", localUUID, "");
}
void PXMPeerWorker::requestIps(evutil_socket_t s, QUuid uuid)
{
    waitingOnIpsFrom = uuid;
    qDebug() << "Requesting ips from" << s;
    emit sendMsg(s, "", "/request", localUUID, "");
}
void PXMPeerWorker::attemptConnection(sockaddr_in addr, QUuid uuid)
{
    if(peerDetailsHash.value(uuid).isConnected || uuid.isNull())
    {
        return;
    }
    evutil_socket_t s = socket(AF_INET, SOCK_STREAM, 0);

    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].socketDescriptor = s;
    peerDetailsHash[uuid].isConnected = true;
    peerDetailsHash[uuid].ipAddressRaw = addr;
    evutil_make_socket_nonblocking(s);
    struct bufferevent *bev = bufferevent_socket_new(PXMServer::base, s, 0);
    emit connectToPeer(s, addr, (void*)bev);
}
void PXMPeerWorker::setPeerHostname(QString hname, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid))
    {
        peerDetailsHash[uuid].hostname = hname;
        emit updateListWidget(uuid);
    }
    return;
}
void PXMPeerWorker::peerQuit(evutil_socket_t s, void *bevptr)
{
    bufferevent *bev = static_cast<bufferevent*>(bevptr);
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == s)
        {
            peerDetailsHash[itr.identifier].isConnected = false;
            peerDetailsHash[itr.identifier].isAuthenticated = false;
            bufferevent_free(bev);
            evutil_closesocket(itr.socketDescriptor);
            peerDetailsHash[itr.identifier].socketDescriptor = -1;
            peerDetailsHash[itr.identifier].bev = nullptr;
            emit setItalicsOnItem(itr.identifier, 1);
            return;
        }
    }
    for(auto &itr : extraBufferevents)
    {
        if(itr == bev)
        {
            extraBufferevents.removeAll(itr);
            bufferevent_free(bev);
        }
    }
    evutil_closesocket(s);
}
size_t PXMPeerWorker::packUuid(char *buf, QUuid *uuid)
{
    int index = 0;

    uint32_t uuidSectionL = htonl((uint32_t)(uuid->data1));
    memcpy(buf + index, &(uuidSectionL), sizeof(uint32_t));
    index += sizeof(uint32_t);
    uint16_t uuidSectionS = htons((uint16_t)(uuid->data2));
    memcpy(buf + index, &(uuidSectionS), sizeof(uint16_t));
    index += sizeof(uint16_t);
    uuidSectionS = htons((uint16_t)(uuid->data3));
    memcpy(buf + index, &(uuidSectionS), sizeof(uint16_t));
    index += sizeof(uint16_t);
    unsigned char *uuidSectionC = uuid->data4;
    memcpy(buf + index, uuidSectionC, 8);
    index += 8;

    return index;
}
void PXMPeerWorker::sendIps(evutil_socket_t i)
{
    qDebug() << "Sending ips to" << i;
    char *msgRaw = new char[peerDetailsHash.size() * (sizeof(uint32_t) + sizeof(uint16_t) + PACKED_UUID_BYTE_LENGTH)];
    //char msgRaw[peerDetailsHash.size() * (sizeof(uint32_t) + sizeof(uint16_t) + PACKED_UUID_BYTE_LENGTH) +1];
    //char msgRaw[peerDetailsHash.size() * (sizeof(uint32_t) + sizeof(uint16_t) + PACKED_UUID_BYTE_LENGTH)];
    //size_t index = 1;
    size_t index = 0;
    //msgRaw[0] = 0;
    for(auto & itr : peerDetailsHash)
    {
        if(itr.isConnected)
        {
            memcpy(msgRaw + index, &(itr.ipAddressRaw.sin_addr.s_addr),sizeof(uint32_t));
            index += sizeof(uint32_t);
            memcpy(msgRaw + index, &(itr.ipAddressRaw.sin_port), sizeof(uint16_t));
            index += sizeof(uint16_t);
            index += this->packUuid(msgRaw + index, &(itr.identifier));
        }
    }
    //emit sendMsg(i, QByteArray::fromRawData(msgRaw, index), "/ip", localUUID, "");
    emit sendIpsPacket(i, msgRaw, index, "/ip", localUUID, "");
}

void PXMPeerWorker::resultOfConnectionAttempt(evutil_socket_t socket, bool result, void *bevptr)
{
    QUuid uuid;
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == socket)
        {
            uuid = itr.identifier;
        }
    }
    if(uuid.isNull())
        return;

    struct bufferevent *bev = static_cast<bufferevent*>(bevptr);
    if(result)
    {
        bufferevent_setcb(bev, PXMServer::tcpReadUUID, NULL, PXMServer::tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, sizeof(uint16_t), sizeof(uint16_t));
        bufferevent_enable(bev, EV_READ);
        if(peerDetailsHash[uuid].bev != nullptr)
            bufferevent_free(peerDetailsHash[uuid].bev);
        peerDetailsHash[uuid].bev = bev;

        emit sendIdentityMsg(socket);
    }
    else
    {
        bufferevent_free(bev);
        evutil_closesocket(socket);
        peerDetailsHash[uuid].isConnected = false;
        peerDetailsHash[uuid].isAuthenticated = false;
        peerDetailsHash[uuid].socketDescriptor = -1;
        peerDetailsHash[uuid].bev = nullptr;
    }
}
void PXMPeerWorker::resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg, bool print)
{
    if(print)
    {
        if(levelOfSuccess < 0)
        {
            msg = QStringLiteral("Message was not sent successfully, Broken Pipe.  Peer likely disconnected");
            //peerQuit(peerDetailsHash.value(uuid).socketDescriptor, peerDetailsHash.value(uuid).bev);
        }
        else if(levelOfSuccess > 0)
        {
            msg.prepend(localHostname % ": ");
            msg.append(QStringLiteral("\nThe previous message was only paritally sent.  This was very bad\nContact the administrator of this program immediately\nNumber of bytes sent: ") % QString::number(levelOfSuccess));
            qDebug() << "Partial Send";
        }
        else
            msg.prepend(localHostname % ": ");
        emit printToTextBrowser(msg, uuid, 0, true);

        return;
    }
    //if(levelOfSuccess<0)
        //peerQuit(peerDetailsHash.value(uuid).socketDescriptor, peerDetailsHash.value(uuid).bev);
}
void PXMPeerWorker::authenticationReceived(QString hname, unsigned short port, evutil_socket_t s, QUuid uuid, void *bevptr)
{
    if(uuid.isNull())
    {
        evutil_closesocket(s);
        bufferevent_free(static_cast<bufferevent*>(bevptr));
        return;
    }
    bufferevent *bev = static_cast<bufferevent*>(bevptr);
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    memset(&addr, 0, socklen);

    getpeername(s, (struct sockaddr*)&addr, &socklen);
    addr.sin_port = htons(port);

    qDebug() << hname << "on port" << QString::number(port) << "authenticated!";

    if(peerDetailsHash[uuid].bev != nullptr && peerDetailsHash[uuid].bev != bev)
    {
        extraBufferevents.append(peerDetailsHash[uuid].bev);
    }
    peerDetailsHash[uuid].bev = bev;
    peerDetailsHash[uuid].socketDescriptor = s;
    peerDetailsHash[uuid].isConnected = true;
    peerDetailsHash[uuid].isAuthenticated = true;
    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].hostname = hname;
    peerDetailsHash[uuid].ipAddressRaw = addr;

    emit updateListWidget(uuid);
    emit requestIps(s, uuid);
}
