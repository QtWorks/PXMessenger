#include <mess_serv.h>

MessengerServer::MessengerServer(QWidget *parent) : QThread(parent)
{

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
int MessengerServer::accept_new(int s, sockaddr_storage *their_addr)
{
    int result;

    socklen_t addr_size = sizeof(sockaddr);
    result = (accept(s, (struct sockaddr *)&their_addr, &addr_size));

    emit newConnectionRecieved(result);

    this->update_fds(result);
    return result;
}
void MessengerServer::retryDiscover()
{
    emit sendUdp("/discover:" + ourListenerPort);
}
void MessengerServer::updateMessServFDSSlot(int s)
{
    this->update_fds(s);
}
/**
 * @brief 				Add an FD to the set if its not already in there and check if its the new max
 * @param s				Socket descriptor number to add to set
 */
void MessengerServer::update_fds(int s)
{
    if( !( FD_ISSET(s, &master) ) )
    {
        FD_SET(s, &master);
        this->set_fdmax(s);
    }
}
/**
 * @brief 				Determine if a new socket descriptor is the highest in the set, adjust the fd_max variable accordingly.
 * @param s				Socket descriptor to potentially make the new max.  fd_max is needed for select in the listener function
 * @return 				0 if it is the new max, -1 if it is not;
 */
int MessengerServer::set_fdmax(int s)
{
    if(s > fdmax)
    {
        fdmax = s;
        return 0;
    }
    return -1;
}
/**
 * @brief 				Called when the TCP listener recieves a new connection
 * @param i				Socket descriptor of new connection
 * @return 				0 on success, 1 on error
 */
int MessengerServer::newConnection(int i)
{
#ifdef _WIN32
    unsigned new_fd;
#else
    int new_fd;
#endif
    struct sockaddr_storage their_addr;

    new_fd = accept_new(i, &their_addr);

#ifdef __unix__
    if(new_fd == -1)
    {
        perror("accept:");
        return 1;
    }
#endif
#ifdef _WIN32
    if(new_fd == INVALID_SOCKET)
    {
        perror("accept:");
        return 1;
    }
#endif
    else
    {
        this->update_fds(new_fd);
        std::cout << "new connection" << std::endl;
        return 0;
    }
    return 1;
}
/**
 * @brief 				Message recieved from a previously connected TCP socket
 * @param i				Socket descriptor to recieve data from
 * @return 				0 on success, 1 on error
 */
int MessengerServer::tcpRecieve(int i)
{
    int nbytes;
    char buf[1050] = {};

    if((nbytes = recv(i,buf,sizeof(buf), 0)) <= 0)
    {
        //The tcp socket is telling us it has been closed
        //If nbytes == 0, normal close
        if(nbytes == 0)
        {
            std::cout << "connection closed" << std::endl;
        }
        //Error
        else
        {
            perror("recv");
        }
        //Either way, alert the main thread this socket has closed.  It will update the peer_class
        //and will make the list item italics to signal a disconnected peer
        FD_CLR(i, &master);
        emit peerQuit(i);
#ifdef __unix__
        close(i);
#endif
#ifdef _WIN32
        closesocket(i);
#endif
        //Remove the socket from the list to check in select
        return 1;
    }
    //Normal message coming here
    else
    {
        //These are variable for determining the ip address of the sender
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        char ipstr[INET6_ADDRSTRLEN];
        char service[20];

        //Get ip address of sender
        getpeername(i, (struct sockaddr*)&addr, &socklen);
        getnameinfo((struct sockaddr*)&addr, socklen, ipstr, sizeof(ipstr), service, sizeof(service), NI_NUMERICHOST);

        this->singleMessageIterator(i, buf, ipstr);
    }
    return 1;
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
    bufLenStr[0] = partialMsg[0];
    bufLenStr[1] = partialMsg[1];
    bufLenStr[2] = partialMsg[2];
    bufLenStr[3] = '\0';
    //.char *bufLenStrTemp = bufLenStr;
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
        emit recievedUUIDForConnection(hpsplit[0], QString::fromUtf8(ipstr), hpsplit[1], i, quuid);
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
int MessengerServer::udpRecieve(int i)
{
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
        char name[50] = {};
        char fname[60] = {};
        strcpy(name, ourListenerPort.toStdString().c_str());
        strcat(name, localUUID.toStdString().c_str());
        strcpy(fname, "/name:\0");
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

        emit updNameRecieved(portNumber, ipAddress, uuid);
    }
    else
    {
        return 1;
    }

    return 0;
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

    if(bind(socketUDP, (sockaddr *)&si_me, sizeof(sockaddr)))
    {
        perror("Binding datagram socket error");
        close(socketUDP);
        exit(1);
    }

    ourListenerPort = QString::number(getPortNumber(s_listen));
    emit setListenerPort(ourListenerPort);

    qDebug() << "Port number for Multicast: " << getPortNumber(socketUDP);

    multicastGroup.imr_multiaddr.s_addr = inet_addr("239.1.1.1");
    multicastGroup.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(socketUDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup));

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
    if(bind(socketTCP, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("bind error: ");
    }
    if(listen(socketTCP, BACKLOG) < 0)
    {
        perror("listen error: ");
    }

    qDebug() << "Port number for TCP/IP Listner" << getPortNumber(socketTCP);

    freeaddrinfo(res);

    return socketTCP;

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

    //TCP STUFF
    s_listen = setupTCPSocket();

    //UDP STUFF
    s_discover = setupUDPSocket(s_listen);

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(s_listen, &master);
    FD_SET(s_discover, &master);

    this->set_fdmax(s_listen);
    this->set_fdmax(s_discover);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    //END of setup for sockets, being infinite while loop to listen.
    //Select is used with a time limit to enable the main thread to close
    //this thread with a signal.
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
    return -1;
}
