#ifndef MESS_CLIENT_H
#define MESS_CLIENT_H

#include <iostream>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <QWidget>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define PORT "13653"

class mess_client : public QObject
{
    Q_OBJECT
public:
    mess_client();
    int c_connect(int socketfd, const char *ipaddr);					//Connect a socket to and ip address
    int send_msg(int socketfd, const char *msg, const char *host, const char *type);	//send a message through an already connected socket to the specified ip address
    int partialSend(int socketfd, const char *msg, int len, int count);			//deal with the kernel not sending all of our message in one go
private:
private slots:
    void sendNameSlot(int);
};

#endif // MESS_CLIENT_H
