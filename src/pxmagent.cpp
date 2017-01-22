#include "pxmagent.h"
#include "pxmmainwindow.h"
#include "pxmpeerworker.h"
#include "pxminireader.h"
#include "pxmconsole.h"

#include <QThread>
#include <QSplashScreen>
#include <QElapsedTimer>
#include <QPixmap>
#include <QMessageBox>
#include <QLockFile>
#include <QDir>
#include <QAbstractButton>
#include <QDebug>
#include <QApplication>
#include <QSharedPointer>
#include <QScopedPointer>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <lmcons.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

Q_DECLARE_METATYPE(QSharedPointer<QString>)

struct initialSettings{
    QUuid uuid;
    QSize windowSize;
    QString username;
    QString multicast;
    int uuidNum;
    unsigned short tcpPort;
    unsigned short udpPort;
    bool mute;
    bool preventFocus;
    initialSettings() : uuid(QUuid()), windowSize(QSize(800,600)), username(QString()),
        multicast(QString()), uuidNum(0), tcpPort(0), udpPort(0), mute(false),
        preventFocus(false)
    {
    }
};

struct PXMAgentPrivate {
    QScopedPointer<PXMWindow> window;
    PXMPeerWorker *peerWorker;
    PXMIniReader iniReader;
    PXMConsole::LoggerSingleton *logger;

    initialSettings presets;

    QThread *workerThread;

    //Functions
    QByteArray getUsername();
    QString setupHostname(int uuidNum, QString username);
};

PXMAgent::PXMAgent(QObject *parent) : QObject(parent), d_ptr(new PXMAgentPrivate)
{

}

PXMAgent::~PXMAgent()
{
    if(d_ptr->workerThread)
    {
        d_ptr->workerThread->quit();
        d_ptr->workerThread->wait(3000);
        qInfo() << "Shutdown of WorkerThread";
    }

    d_ptr->iniReader.resetUUID(d_ptr->presets.uuidNum, d_ptr->presets.uuid);
    d_ptr->iniReader.setVerbosity(d_ptr->logger->getVerbosityLevel());
}

int PXMAgent::init()
{
#ifndef QT_DEBUG
    QPixmap splashImage(":/resources/resources/PXMessenger_wBackground.png");
    splashImage = splashImage.scaledToHeight(300);
    QSplashScreen splash(splashImage);
    splash.show();
    QElapsedTimer startupTimer;
    startupTimer.start();
    qApp->processEvents();
#endif

#ifdef _WIN32
    qRegisterMetaType<intptr_t>("intptr_t");
#endif
    qRegisterMetaType<sockaddr_in>();
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<bufferevent*>();
    qRegisterMetaType<PXMConsts::MESSAGE_TYPE>();
    qRegisterMetaType<QSharedPointer<Peers::BevWrapper>>();
    qRegisterMetaType<QSharedPointer<QString>>();

    QString username = d_ptr->getUsername();
    QString localHostname = d_ptr->iniReader.getHostname(username);

    bool allowMoreThanOne = d_ptr->iniReader.checkAllowMoreThanOne();
    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/pxmessenger_" + username + ".lock");
    if(!allowMoreThanOne)
    {
        if(!lockFile.tryLock(100))
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("You already have this app running."
                           "\r\nOnly one instance is allowed.");
            msgBox.exec();
            return -1;
        }
    }

    d_ptr->logger = PXMConsole::LoggerSingleton::getInstance();
    d_ptr->logger->setVerbosityLevel(d_ptr->iniReader.getVerbosity());
    d_ptr->presets.uuidNum = d_ptr->iniReader.getUUIDNumber();
    d_ptr->presets.uuid = d_ptr->iniReader.getUUID(d_ptr->presets.uuidNum, allowMoreThanOne);
    d_ptr->presets.username = d_ptr->setupHostname(d_ptr->presets.uuidNum, localHostname.left(PXMConsts::MAX_HOSTNAME_LENGTH));
    d_ptr->presets.tcpPort = d_ptr->iniReader.getPort("TCP");
    d_ptr->presets.udpPort = d_ptr->iniReader.getPort("UDP");
    d_ptr->presets.windowSize = d_ptr->iniReader.getWindowSize(QSize(700, 500));
    d_ptr->presets.mute = d_ptr->iniReader.getMute();
    d_ptr->presets.preventFocus = d_ptr->iniReader.getFocus();
    d_ptr->presets.multicast = d_ptr->iniReader.getMulticastAddress();
    QUuid globalChat = QUuid::createUuid();
    if(!(d_ptr->iniReader.getFont().isEmpty()))
    {
        QFont font;
        font.fromString(d_ptr->iniReader.getFont());
        qApp->setFont(font);
    }

    d_ptr->workerThread = new QThread(this);
    d_ptr->workerThread->setObjectName("WorkerThread");
    d_ptr->peerWorker = new PXMPeerWorker(nullptr, d_ptr->presets.username,
                                               d_ptr->presets.uuid, d_ptr->presets.multicast,
                                               d_ptr->presets.tcpPort, d_ptr->presets.udpPort,
                                               globalChat);
    d_ptr->peerWorker->moveToThread(d_ptr->workerThread);
    QObject::connect(d_ptr->workerThread, &QThread::started, d_ptr->peerWorker, &PXMPeerWorker::currentThreadInit);
    QObject::connect(d_ptr->workerThread, &QThread::finished, d_ptr->peerWorker, &PXMPeerWorker::deleteLater);
    QScopedPointer<PXMWindow> win(new PXMWindow(d_ptr->presets.username, d_ptr->presets.windowSize,
                                      d_ptr->presets.mute, d_ptr->presets.preventFocus, globalChat));
    d_ptr->window.swap(win);
    //d_ptr->window = new PXMWindow(d_ptr->presets.username, d_ptr->presets.windowSize,
    //                                  d_ptr->presets.mute, d_ptr->presets.preventFocus, globalChat);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::printToTextBrowser, d_ptr->window.data(), &PXMWindow::printToTextBrowser, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::setItalicsOnItem, d_ptr->window.data(), &PXMWindow::setItalicsOnItem, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::updateListWidget, d_ptr->window.data(), &PXMWindow::updateListWidget, Qt::QueuedConnection);
    QObject::connect(d_ptr->peerWorker, &PXMPeerWorker::warnBox, d_ptr->window.data(), &PXMWindow::warnBox, Qt::AutoConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::addMessageToPeer, d_ptr->peerWorker, &PXMPeerWorker::addMessageToPeer, Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::sendMsg, d_ptr->peerWorker, &PXMPeerWorker::sendMsgAccessor, Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::sendUDP, d_ptr->peerWorker, &PXMPeerWorker::sendUDPAccessor, Qt::QueuedConnection);
    QObject::connect(d_ptr->window.data(), &PXMWindow::printInfoToDebug, d_ptr->peerWorker, &PXMPeerWorker::printInfoToDebug, Qt::QueuedConnection);
    d_ptr->workerThread->start();

#ifdef QT_DEBUG
    qInfo() << "Running in debug mode";
#else
    qInfo() << "Running in release mode";
    splash.finish(d_ptr->window);
    while(startupTimer.elapsed() < 1500)
    {
        qApp->processEvents();
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100.000);
#endif
    }
#endif

    qInfo() << "Our UUID:" << d_ptr->presets.uuid.toString();

    d_ptr->window->show();
    return 0;
}

QByteArray PXMAgentPrivate::getUsername()
{
#ifdef _WIN32
    size_t len = UNLEN+1;
    char localHostname[len];
    TCHAR t_user[len];
    DWORD user_size = len;
    if(GetUserName(t_user, &user_size))
    {
        wcstombs(localHostname, t_user, len);
        return QByteArray(localHostname);
    }
    else
        return QByteArray("user");
#else
    struct passwd *user;
    user = getpwuid(getuid());
    if(!user)
        return QByteArray("user");
    else
        return QByteArray(user->pw_name);
#endif
}

QString PXMAgentPrivate::setupHostname(int uuidNum, QString username)
{
    char computerHostname[256] = {};

    gethostname(computerHostname, sizeof computerHostname);

    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        username.append(QString::fromUtf8(temp));
    }
    username.append("@");
    username.append(QString::fromLocal8Bit(computerHostname).left(PXMConsts::MAX_COMPUTER_NAME));
    return username;
}
