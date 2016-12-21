#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QFontDatabase>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pxmmainwindow.h"
#include "pxmdebugwindow.h"
#include "pxminireader.h"
#include "pxmdefinitions.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

static_assert(sizeof(uint8_t) == 1, "uint8_t not defined as 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

int AppendTextEvent::type = QEvent::registerEventType();
LoggerSingleton* LoggerSingleton::loggerInstance = nullptr;

void debugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
#ifdef QT_DEBUG
    QByteArray localMsg;
    QString filename = QString::fromUtf8(context.file);
    filename = filename.right(filename.length() - filename.lastIndexOf(QChar('/')) - 1);
    filename.append(":" + QByteArray::number(context.line));
    filename.append(QString(PXMConsts::DEBUG_PADDING - filename.length(), QChar(' ')));
    localMsg = filename.toUtf8() + msg.toUtf8();
#else
    QByteArray localMsg = msg.toLocal8Bit();
#endif
    switch(type) {
    case QtDebugMsg:
        fprintf(stdout, "%s\n", localMsg.constData());
        //fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    }
    if(PXMDebugWindow::textEdit != 0)
    {
        LoggerSingleton *logger = LoggerSingleton::getInstance();
        qApp->postEvent(logger, new AppendTextEvent(localMsg), Qt::LowEventPriority);
    }
}

char* getHostname()
{
#ifdef _WIN32
    size_t len = UNLEN+1;
    char* localHostname = new char[len];
    memset(localHostname, 0, len);
    TCHAR t_user[len];
    DWORD user_size = len;
    if(GetUserName(t_user, &user_size))
        wcstombs(localHostname, t_user, len);
    else
        strcpy(localHostname, "user");
#else
    size_t len = sysconf(_SC_GETPW_R_SIZE_MAX);
    char* localHostname = new char[len];
    memset(localHostname, 0, len);
    struct passwd *user;
    user = getpwuid(getuid());
    if(!user)
        strcpy(localHostname, "user");
    else
        strcpy(localHostname, user->pw_name);
#endif

    return localHostname;
}

int main(int argc, char **argv)
{
    qInstallMessageHandler(debugMessageOutput);
#ifdef _WIN32
    qRegisterMetaType<intptr_t>("intptr_t");
#endif
    qRegisterMetaType<sockaddr_in>();
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<bufferevent*>();
    qRegisterMetaType<PXMConsts::MESSAGE_TYPE>();

    QApplication app (argc, argv);

#ifdef _WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
        printf("Failed WSAStartup error: %d", WSAGetLastError());
        return 1;
    }
#endif

    QApplication::setApplicationName("PXMessenger");
    QApplication::setOrganizationName("PXMessenger");
    QApplication::setOrganizationDomain("PXMessenger");
    QApplication::setApplicationVersion("1.2.3");

    MessIniReader iniReader;
    initialSettings presets;

    QFont font;
    if(!(iniReader.getFont().isEmpty()))
    {
        font.fromString(iniReader.getFont());
        app.setFont(font);
    }

    char* localHostname = getHostname();

    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + "/pxmessenger" + QString::fromLatin1(localHostname) + ".lock");

    bool allowMoreThanOne = iniReader.checkAllowMoreThanOne();

    if(allowMoreThanOne)
    {
        presets.uuidNum = iniReader.getUUIDNumber();
    }
    else
    {
        if(!lockFile.tryLock(100))
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("You already have this app running."
                           "\r\nOnly one instance is allowed.");
            msgBox.exec();
            return 1;
        }
    }
    presets.uuid = iniReader.getUUID(presets.uuidNum, allowMoreThanOne);
    presets.username = iniReader.getHostname(QString::fromLatin1(localHostname));
    presets.tcpPort = iniReader.getPort("TCP");
    presets.udpPort = iniReader.getPort("UDP");
    presets.windowSize = iniReader.getWindowSize(QSize(700, 500));
    presets.mute = iniReader.getMute();
    presets.preventFocus = iniReader.getFocus();
    presets.multicast = iniReader.getMulticastAddress();

    int result;
    {
        PXMWindow window(presets);
        window.startThreadsAndShow();
#ifdef QT_DEBUG
        qDebug() << "Running in debug mode";
#else
        qDebug() << "Running in release mode";
#endif
        result = app.exec();
    }

    iniReader.resetUUID(presets.uuidNum, presets.uuid);

    delete[] localHostname;

    qDebug() << "Exiting PXMessenger";

    return result;
}
