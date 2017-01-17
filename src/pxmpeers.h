#pragma once
#ifndef PXMPEERS_H
#define PXMPEERS_H

#include <QUuid>
#include <QSize>
#include <QLinkedList>
#include <QVector>
#include <QMutex>
#include <QSharedPointer>

//#include "uuidcompression.h"
#include <event2/bufferevent.h>

Q_DECLARE_METATYPE(sockaddr_in)
Q_DECLARE_METATYPE(size_t)
Q_DECLARE_OPAQUE_POINTER(bufferevent*)
Q_DECLARE_METATYPE(bufferevent*)

namespace Peers{
const QString selfColor = "#6495ED"; //Cornflower Blue
const QString peerColor = "#FF0000"; //Red
const QVector<QString> textColors = {"#808000", //Olive
                                     "#FFA500", //Orange
                                     "#FF00FF", //Fuschia
                                     "#DC143C", //Crimson
                                     "#FF69B4", //HotPink
                                     "#708090", //SlateGrey
                                     "#008000", //Green
                                     "#00FF00"}; //Lime
extern int textColorsNext;
class BevWrapper {
public:
    //Default Constructor
    BevWrapper() : bev(nullptr), locker(new QMutex) {}
    //Constructor with a bufferevent
    BevWrapper(bufferevent *buf) : bev(buf), locker(new QMutex) {}
    //Destructor
    ~BevWrapper();
    //Copy Constructor
    BevWrapper(const BevWrapper& b) : bev(b.bev), locker(b.locker) {}
    //Move Constructor
    BevWrapper(BevWrapper&& b) noexcept : bev(b.bev), locker(b.locker)
    {
        b.bev = nullptr;
        b.locker = nullptr;
    }
    //Move Assignment
    BevWrapper &operator =(BevWrapper&& b) noexcept;
    //Eqaul
    bool operator ==(const BevWrapper& b) {return bev == b.bev;}
    //Not Equal
    bool operator !=(const BevWrapper& b) {return !(bev == b.bev);}

    void setBev(bufferevent *buf) {bev = buf;}
    bufferevent* getBev() {return bev;}
    void lockBev() {locker->lock();}
    void unlockBev() {locker->unlock();}
    int freeBev();

private:
    bufferevent *bev;
    QMutex *locker;
};

struct PeerData{
    QUuid identifier;
    sockaddr_in ipAddressRaw;
    QString hostname;
    QString textColor;
    QLinkedList<QSharedPointer<QString>> messages;
    QSharedPointer<BevWrapper> bw;
    evutil_socket_t socket;
    bool connectTo;
    bool isAuthenticated;

    //Default Constructor
    PeerData();

    //Copy
    PeerData (const PeerData& pd) : identifier(pd.identifier),
            ipAddressRaw(pd.ipAddressRaw), hostname(pd.hostname),
            textColor(pd.textColor),
            messages(pd.messages), bw(pd.bw), socket(pd.socket),
            connectTo(pd.connectTo), isAuthenticated(pd.isAuthenticated) {}

    //Move
    PeerData (PeerData&& pd) noexcept : identifier(pd.identifier),
            ipAddressRaw(pd.ipAddressRaw), hostname(pd.hostname),
            textColor(pd.textColor),
            messages(pd.messages), bw(pd.bw), socket(pd.socket),
            connectTo(pd.connectTo), isAuthenticated(pd.isAuthenticated)
    {
        pd.bw.clear();
    }

    //Destructor
    ~PeerData() noexcept {}

    //Move assignment
    PeerData& operator= (PeerData&& pd) noexcept;

    //Copy assignment
    PeerData& operator= (const PeerData& pd);

    //Return data of this struct as a string padded with the value in 'pad'
    QString toInfoString();
};

}
Q_DECLARE_METATYPE(QSharedPointer<Peers::BevWrapper>)

struct initialSettings{
    int uuidNum;
    unsigned short tcpPort;
    unsigned short udpPort;
    bool mute;
    bool preventFocus;
    QString username;
    QSize windowSize;
    QUuid uuid;
    QString multicast;
    initialSettings()
    {
        uuidNum = 0;
        tcpPort = 0;
        udpPort = 0;
        mute = false;
        preventFocus = false;
        username = QString();
        windowSize = QSize(800,600);
        uuid = QUuid();
        multicast = QString("239.192.13.13"); //FIXME:USE CONSTS
    }
};

#endif
