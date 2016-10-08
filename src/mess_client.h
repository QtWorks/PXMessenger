#ifndef MESS_CLIENT_H
#define MESS_CLIENT_H


class mess_client
{
public:
    mess_client();
    int c_connect(int socketfd, const char *ipaddr);					//Connect a socket to and ip address
    int send_msg(int socketfd, const char *msg, const char *host);	//send a message through an already connected socket to the specified ip address
    //char *msg = {};															//unknown
    int partialSend(int socketfd, const char *msg, int len, int count);			//deal with the kernel not sending all of our message in one go
};

#endif // MESS_CLIENT_H