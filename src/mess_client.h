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
#include <QUuid>

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

class MessengerClient : public QObject
{
    Q_OBJECT
public:
    MessengerClient();
    int c_connect(int socketfd, const char *ipaddr);					//Connect a socket to and ip address
    int send_msg(int socketfd, const char *msg, const char *host, const char *type, const char *uuid);	//send a message through an already connected socket to the specified ip address
public slots:
    void sendMsgSlot(int s, QString msg, QString host, QString type, QUuid uuid);
    void connectToPeerSlot(int s, QString ipaddr);
    void udpSendSlot(QString msg);
private:
    int partialSend(int socketfd, const char *msg, int len, int count);			//deal with the kernel not sending all of our message in one go
    void udpSend(const char *msg);
private slots:
    void sendNameSlot(int, QString uuid);
signals:
    void resultOfConnectionAttempt(int, bool);
    void resultOfTCPSend(int, QString, QString, bool);
};

#endif // MESS_CLIENT_H
