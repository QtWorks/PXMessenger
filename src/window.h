#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QTextBrowser>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QAction>
#include <QWindow>
#include <QScrollBar>
#include <QDebug>
#include <QCloseEvent>
#include <QSound>
#include <QAction>
#include <QApplication>
#include <QPalette>

#include <sys/types.h>
#include <ctime>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <iostream>
#include <sstream>
#include <sys/fcntl.h>

#include <mess_serv.h>
//#include <mess_discover.h>
#include <mess_textedit.h>
#include <mess_client.h>
#include <peerlist.h>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#define PORT "13653"

class MessengerWindow : public QWidget
{
    Q_OBJECT

public:
    MessengerWindow();

protected:
    void closeEvent(QCloseEvent *event)  Q_DECL_OVERRIDE;									//Close event handler
    void changeEvent(QEvent *event);
private slots:
    void sendButtonClicked();													//Send button event
    void quitButtonClicked();														//quit button, used to test the various destructors and close event
    void currentItemChanged(QListWidgetItem *item1, QListWidgetItem *item2);//change text in textbrowser to show correct text history and active send recipient
    void textEditChanged();
    void showWindow(QSystemTrayIcon::ActivationReason reason);
    void printToTextBrowserServerSlot(const QString str, const QString ipstr, bool global);					//calls print, currently identifys recipient based on socket descriptor, needs revision
    void printToTextBrowser(QString str, int peerindex, bool message);					//Updates the main text box and stores text history in peerlist struct

    //void listpeers(QString hname, QString ipaddr);
    //void listpeers(QString hname, QString ipaddr, bool test, int s);							//Add new peers to the peerlist struct array and call the sort/display functions
    //void new_client(int s, QString ipaddr);									//Networking
    //void peerQuit(int s);													//Connected client disconnected
    //void potentialReconnect(QString ipaddr);								//client has sent a discover udp to us, test if we have seen him before
    //void sendIps(int i);
    //void hostnameCheck(QString comp);
    //void setPeerHostname(QString hname, QString ipaddr);
    void updateListWidget(int num);														//sort peerlist struct alphabetically by hostname
    void setItalicsOnItem(int i, bool italics);
private:
    QPushButton *messSendButton;
    QPushButton *messQuitButton;
    QIcon *messSystemTrayIcon;
    QAction *messSystemTrayExitAction;
    QMenu *messSystemTrayMenu;
    QSystemTrayIcon *messSystemTray;
    QTextBrowser *messTextBrowser;
    QLineEdit *messLineEdit;
    QPushButton *m_sendDebugButton;
    QListWidget *messListWidget;
    QThread *messClientThread;

    MessengerTextEdit *messTextEdit;
    MessengerClient *messClient;
    MessengerServer *messServer;
    PeerClass *peers_class;

    time_t messTime;
    struct tm *currentTime;

    int numberOfValidPeers = 0;														//Length of peers array
    char localHostname[128] = {};
    QString globalChat = "";
    int globalChatIndex = 1;
    bool globalChatAlerted = false;

    //void assignSocket(struct peerDetails *p);									//Give socket value to peerlist array member
    void udpSend(const char *msg);											//send a UDP discover request to the broadcast address of the network
    void globalSend(QString msg);

    void focusWindow();
    void removeMessagePendingStatus(QListWidgetItem *item);
    void changeListColor(int row, int style);

    void createTextEdit();
    void createTextBrowser();
    void createLineEdit();
    void createButtons();
    void createListWidget();
    void createSystemTray();
    void connectGuiSignalsAndSlots();
    void createMessClient();
    void createMessServ();
    void createMessTime();
    void connectPeerClassSignalsAndSlots();
signals:
    void connectToPeer(int, QString);
    void sendMsg(int, QString, QString, QString);

};

#endif // WINDOW_H
