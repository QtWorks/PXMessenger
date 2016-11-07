#include <mess_serv.h>

MessengerServer::MessengerServer(QWidget *parent) : QThread(parent)
{
    tcpBuffer = new char[TCP_BUFFER_LENGTH];
}
MessengerServer::~MessengerServer()
{
    delete [] tcpBuffer;
}

void MessengerServer::setLocalHostname(QString hostname)
{
    localHostname = hostname;
}
void MessengerServer::setLocalUUID(QString uuid)
{
    localUUID = uuid;
}
/**
 * @brief				Start of thread, call the listener function which is an infinite loop
 */
void MessengerServer::run()
{
    this->listener();
}
/**
 * @brief 				Accept a new TCP connection from the listener socket and let the GUI know.
 * @param s				Listener socket on from which we accept a new connection
 * @param their_addr
 * @return 				Return socket descriptor for new connection, -1 on error on linux, INVALID_SOCKET on Windows
 */
void MessengerServer::accept_new(evutil_socket_t s, short event, void *arg)
{
    int result;
    qDebug() << event;

    MessengerServer *realServer = (MessengerServer*)arg;

    struct event_base *base = realServer->base;
    struct sockaddr_storage ss;
    socklen_t addr_size = sizeof(ss);

    result = (accept(s, (struct sockaddr *)&ss, &addr_size));
    if(result < 0)
    {
        perror("accept");
    }
    else if(result > FD_SETSIZE)
    {
        close(result);
    }
    else
    {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(result);
        bev = bufferevent_socket_new(base, result, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, tcpRead, NULL, tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, 0, TCP_BUFFER_LENGTH);
        bufferevent_enable(bev, EV_READ);
        realServer->newConnectionRecieved(result);
    }


    //emit newConnectionRecieved(result);
}
void MessengerServer::tcpRead(struct bufferevent *bev, void *ctx)
{
    MessengerServer *realServer = (MessengerServer *)ctx;
    struct evbuffer *input;
    char *line = new char[1000];
    memset(line, 0, 1000*sizeof(*line));
    evutil_socket_t i;
    input = bufferevent_get_input(bev);
    evbuffer_remove(input, line, 1000);
    //qDebug() << line;
    i = bufferevent_getfd(bev);

    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    char ipstr[INET6_ADDRSTRLEN];
    char service[20];

    //Get ip address of sender
    getpeername(i, (struct sockaddr*)&addr, &socklen);
    getnameinfo((struct sockaddr*)&addr, socklen, ipstr, sizeof(ipstr), service, sizeof(service), NI_NUMERICHOST);
    realServer->singleMessageIterator(i, line, ipstr);
    delete [] line;
}
void MessengerServer::tcpError(struct bufferevent *buf, short error, void *ctx)
{
    MessengerServer *realServer = (MessengerServer *)ctx;
    evutil_socket_t i = bufferevent_getfd(buf);
    if (error & BEV_EVENT_EOF)
    {
        realServer->peerQuit(i);
    }
    else if (error & BEV_EVENT_ERROR)
    {

        /* check errno to see what error occurred */
        /* ... */
    } else if (error & BEV_EVENT_TIMEOUT) {
        /* must be a timeout event handle, handle it */
        /* ... */
    }
    bufferevent_free(buf);
}
int MessengerServer::singleMessageIterator(int i, char *buf, char *ipstr)
{
    //partialMsg points to the start of the original buffer.
    //If there is more than one message on it, it will be increased to where the
    //next message begins
    char *partialMsg = buf;

    //This holds the first 3 characters of partialMsg which will represent how long the recieved message should be.
    char bufLenStr[4] = {};

    //The first three characters of each message should be the length of the message.
    //We parse this to an integer so as not to confuse messages with one another
    //when more than one are recieved from the same socket at the same time
    //If we encounter a valid character after this length, we come back here
    //and iterate through the if statements again.
    strncpy(bufLenStr, partialMsg, 3);

    int bufLen = atoi(bufLenStr);
    bufLen -= 38;
    if(bufLen <= 0)
        return 1;
    partialMsg = partialMsg+3;
    //skip over the UUID, not checking this yet, initial stages
    //FIX THIS AFTER MORE FUNCTIONALITY
    QUuid quuid = QString::fromUtf8(partialMsg, 38);
    if(quuid.isNull())
        return -1;
    partialMsg += 38;


    //These packets should be formatted like "/msghostname: msg\0"
    if( !( strncmp(partialMsg, "/msg", 4) ) )
    {
        //Copy the string in buf starting at buf[4] until we reach the length of the message
        //We dont want to keep the "/msg" substring, hence the +/- 4 here.
        //Remember, we added 3 to buf above to ignore the size of the message
        //buf is actually at buf[7] from its original location

        //The following signal is going to the main thread and will call the slot prints(QString, QString)
        emit messageRecieved(QString::fromUtf8(partialMsg+4, bufLen-4), quuid, false);
    }
    //These packets should come formatted like "/ip:hostname@192.168.1.1:hostname2@192.168.1.2\0"
    else if( !( strncmp(partialMsg, "/ip", 3) ) )
    {
        qDebug() << "/ip recieved from " << QString::fromUtf8(ipstr);
        int count = 0;

        //We need to extract each hostname and ip set out of the message and send them to the main thread
        //to check if whether we already know about them or not
        for(int k = 4; k < bufLen; k++)
        {
            //Theres probably a better way to do this
            if(partialMsg[k] == '[' || partialMsg[k] == ']')
            {
                char temp[INET_ADDRSTRLEN + 5 + 38] = {};
                strncpy(temp, partialMsg+(k-count), count);
                count = 0;
                if((strlen(temp) < 2))
                    *temp = 0;
                else
                    //The following signal is going to the main thread and will call the slot ipCheck(QString)
                    emit hostnameCheck(QString::fromUtf8(temp));
            }
            else
                count++;
        }
    }
    else if(!(strncmp(partialMsg, "/uuid", 5)))
    {
        qDebug() << "uuid recieved: " + quuid.toString();
        QString hostandport = QString::fromUtf8(partialMsg+5, bufLen-5);
        QStringList hpsplit = hostandport.split(":");
        emit recievedUUIDForConnection(hpsplit[0], QString::fromUtf8(ipstr, strlen(ipstr)), hpsplit[1], i, quuid);
    }
    //This packet is asking us to communicate our list of peers with the sender, leads to us sending an /ip packet
    //These packets should come formatted like "/request\0"
    else if(!(strncmp(partialMsg, "/request", 8)))
    {
        qDebug() << "/request recieved from " << QString::fromUtf8(ipstr) << "\nsending ips";
        //The following signal is going to the m_client object and thread and will call the slot sendIps(int)
        //The int here is the socketdescriptor we want to send our ip set too.
        emit sendIps(i);
    }
    //These packets are messages sent to the global chat room
    //These packets should come formatted like "/globalhostname: msg\0"
    else if(!(strncmp(partialMsg, "/global", 7)))
    {
        //bufLen-6 instead of 7 because we need a trailing NULL character for QString conversion
        char emitStr[bufLen-6] = {};
        strncpy(emitStr, (partialMsg+7), bufLen-7);
        emit messageRecieved(QString::fromUtf8(emitStr), quuid, true);
    }
    //This packet is an updated hostname for the computer that sent it
    //These packets should come formatted like "/hostnameHostname1\0"
    else if(!(strncmp(partialMsg, "/hostname", 9)))
    {
        qDebug() << "/hostname recieved" << QString::fromUtf8(ipstr);
        //bufLen-8 instead of 9 because we need a trailing NULL character for QString conversion
        char emitStr[bufLen-8] = {};
        strncpy(emitStr, (partialMsg+9), bufLen-9);
        emit setPeerHostname(QString::fromUtf8(emitStr), quuid);
    }
    //This packet is asking us to communicate an updated hostname to the sender
    //These packets should come formatted like "/namerequest\0"
    else if(!(strncmp(partialMsg, "/namerequest", 12)))
    {
        qDebug() << "/namerequest recieved from " << QString::fromUtf8(ipstr) << "\nsending hostname";
        emit sendName(i, quuid.toString(), localUUID);
    }
    else
    {
        qDebug() << "Bad message type in the packet, discarding the rest";
        return -1;
    }

    //Check if theres more in the buffer
    if(partialMsg[bufLen] != '\0')
    {
        partialMsg += bufLen;
        this->singleMessageIterator(i, partialMsg, ipstr);
    }
    return 0;
}
/**
 * @brief 				UDP packet recieved. Remember, these are connectionless
 * @param i				Socket descriptor to recieve UDP packet from
 * @return 				0 on success, 1 on error
 */
void MessengerServer::udpRecieve(int i, short int y, void *args)
{
    MessengerServer *realServer = (MessengerServer*)args;
    socklen_t si_other_len;
    sockaddr_in si_other;
    char service[20];
    char buf[1000] = {};
    char ipstr[INET6_ADDRSTRLEN];

    si_other_len = sizeof(sockaddr);
    recvfrom(i, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &si_other_len);
    getnameinfo(((struct sockaddr*)&si_other), si_other_len, ipstr, sizeof(ipstr), service, sizeof(service), NI_NUMERICHOST);
    if (strncmp(buf, "/discover", 9) == 0)
    {
        struct sockaddr_in addr;
        int socket1;
        if ( (socket1 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
            perror("socket:");
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = si_other.sin_addr.s_addr;
        addr.sin_port = htons(PORT_DISCOVER);
        char name[realServer->ourListenerPort.length() + realServer->localUUID.length() + 1] = {};
        char fname[sizeof(name) + 6] = {};
        strcpy(name, realServer->ourListenerPort.toStdString().c_str());
        strcat(name, realServer->localUUID.toStdString().c_str());
        strcpy(fname, "/name:");
        strcat(fname, name);
        int len = strlen(fname);

        for(int k = 0; k < 2; k++)
        {
            sendto(socket1, fname, len+1, 0, (struct sockaddr *)&addr, sizeof(addr));
        }
#ifdef _WIN32
        closesocket(socket1);
#else
        close(socket1);
#endif
    }
    //This will get sent from anyone recieving a /discover packet
    //when this is recieved it add the sender to the list of peers and connects to him
    else if ((strncmp(buf, "/name:", 6)) == 0)
    {
        QString portNumber;
        QString ipAddress;
        char fullid[50];

        strcpy(fullid, &buf[6]);

        QString fullidStr = QString::fromUtf8(fullid);
        portNumber = fullidStr.left(fullidStr.indexOf("{"));
        QString uuid = fullidStr.right(fullidStr.length() - fullidStr.indexOf("{"));
        ipAddress = QString::fromUtf8(ipstr);

        realServer->updNameRecieved(portNumber, ipAddress, uuid);
    }
    else
    {
        return;
    }

    return;
}
int MessengerServer::getPortNumber(int socket)
{
    sockaddr_in needPortNumber;
    memset(&needPortNumber, 0, sizeof(needPortNumber));
    socklen_t needPortNumberLen = sizeof(needPortNumber);
    if(getsockname(socket, (struct sockaddr*)&needPortNumber, &needPortNumberLen) != 0)
    {
        printf("getsockname: %s\n", strerror(errno));
        return -1;
    }
    return ntohs(needPortNumber.sin_port);
}
int MessengerServer::setupUDPSocket(int s_listen)
{
    sockaddr_in si_me;
    ip_mreq multicastGroup;

    int socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT_DISCOVER);
    si_me.sin_addr.s_addr = INADDR_ANY;

    setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, "true", sizeof (int));
    evutil_make_socket_nonblocking(socketUDP);

    if(bind(socketUDP, (sockaddr *)&si_me, sizeof(sockaddr)))
    {
        perror("Binding datagram socket error");
        close(socketUDP);
        exit(1);
    }

    ourListenerPort = QString::number(getPortNumber(s_listen));
    emit setListenerPort(ourListenerPort);

    qDebug() << "Port number for Multicast: " << getPortNumber(socketUDP);

    multicastGroup.imr_multiaddr.s_addr = inet_addr("239.192.13.13");
    multicastGroup.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(socketUDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup));

    /*if(setSocketToNonBlocking(socketUDP) < 0)
    {
        perror("could not set socket to NONBLOCK");
    }
    */
    //send our discover packet to find other computers
    emit sendUdp("/discover:" + ourListenerPort);

    return socketUDP;
}
int MessengerServer::setupTCPSocket()
{
    struct addrinfo hints, *res;
    int socketTCP;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //getaddrinfo(NULL, ourListenerPort.toStdString().c_str(), &hints, &res);
    getaddrinfo(NULL, "0", &hints, &res);

    if((socketTCP = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
        perror("socket error: ");
    }
    if(setsockopt(socketTCP, SOL_SOCKET,SO_REUSEADDR, "true", sizeof(int)) < 0)
    {
        perror("setsockopt error: ");
    }
    
    evutil_make_socket_nonblocking(socketTCP);
    
    if(bind(socketTCP, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("bind error: ");
    }
    if(listen(socketTCP, BACKLOG) < 0)
    {
        perror("listen error: ");
    }
    /*if(setSocketToNonBlocking(socketTCP) < 0)
    {
        perror("could not set socket to NONBLOCK");
    }
    */
    qDebug() << "Port number for TCP/IP Listner" << getPortNumber(socketTCP);

    freeaddrinfo(res);

    return socketTCP;

}
int MessengerServer::setSocketToNonBlocking(int socket)
{
#ifdef _WIN32
    unsigned long ul = 1;
    ioctlsocket(socket, FIONBIO, &ul);
#else
    int flags;
    flags = fcntl(socket, F_GETFL);
    if(flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if(fcntl(socket, F_SETFL, flags) < 0)
        return -1;
    return 0;
#endif
}
/**
 * @brief 				Main listener called from the run function.  Infinite while loop in here that is interuppted by
 *						the GUI thread upon shutdown.  Two listeners, one TCP/IP and one UDP, are created here and checked
 *						for new connections.  All new connections that come in here are listened to after they have had their
 *						descriptors added to the master FD set.
 * @return				Should never return, -1 means something terrible has happened
 */
int MessengerServer::listener()
{
    //Potential rewrite to change getaddrinfo to a manual setup of the socket structures.
    //Changing to manual setup may improve load times on windows systems.  Locks us into
    //ipv4 however.
    int s_discover, s_listen;
    struct event *eventAccept;
    struct event *eventDiscover;


    base = event_base_new();
    if(!base)
        return -1;
    
    //TCP STUFF
    s_listen = setupTCPSocket();

    //UDP STUFF
    s_discover = setupUDPSocket(s_listen);

    eventDiscover = event_new(base, s_discover, EV_READ|EV_PERSIST, udpRecieve, (void*)this);

    event_add(eventDiscover, NULL);

    eventAccept = event_new(base, s_listen, EV_READ|EV_PERSIST, accept_new, (void*)this);
    
    event_add(eventAccept, NULL);

    //emit updNameRecieved(ourListenerPort, "192.168.1.200", localUUID);

    struct timeval halfSec;
    halfSec.tv_sec = 0;
    halfSec.tv_usec = 500000;
    
    while(!this->isInterruptionRequested())
    {
        event_base_loopexit(base, &halfSec);

        event_base_dispatch(base);
    }

    //event_free(eventAccept);
    //event_free(eventDiscover);

    //END of setup for sockets, being infinite while loop to listen.
    //Select is used with a time limit to enable the main thread to close
    //this thread with a signal.
    /*
    while( !this->isInterruptionRequested() )
    {
    read_fds = master;
    int status = 0;
    //This while loop will interrupt on signal from the main thread, or having a socket
    //that has data waiting to be read from.  Select returns -1 on error, 0 on no data
    //waiting on any socket.
    while( ( (status  ==  0) | (status == -1) ) && ( !( this->isInterruptionRequested() ) ) )
    {
        if(status == -1){
        perror("select:");
        }
        read_fds = master;
        status = select(fdmax+1, &read_fds, NULL, NULL, &tv);
        tv.tv_sec = 0;
        tv.tv_usec = 250000; //quarter of a second, select modifies this value so it must be reset before every loop
    }
    for(int i = 0; i <= fdmax; i++)
    {
        if(FD_ISSET(i, &read_fds))
        {
        //New tcp connection signaled from s_listen
        if(i == s_listen)
        {
            this->newConnection(i);
        }
        //UDP packet sent to the udp socket
        else if(i == s_discover)
        {
            this->udpRecieve(i);
        }
        //TCP packet sent to CONNECTED socket
        else
        {
            this->tcpRecieve(i);
        }
        }
    }
    }
    */
    return 0;
}
