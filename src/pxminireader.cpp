#include "pxminireader.h"
#include "pxmconsts.h"
#include <QSettings>
#include <QSize>
#include <QString>
#include <QUuid>
#include <QDebug>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

PXMIniReader::PXMIniReader()
    : inisettings(new QSettings(QSettings::IniFormat, QSettings::UserScope, "PXMessenger", "PXMessenger", NULL))
{
}
PXMIniReader::~PXMIniReader()
{
}
bool PXMIniReader::checkAllowMoreThanOne()
{
    if (inisettings->contains("config/AllowMoreThanOneInstance")) {
        return inisettings->value("config/AllowMoreThanOneInstance", false).toBool();
    }
    inisettings->setValue("config/AllowMoreThanOneInstance", false);
    return false;
}
void PXMIniReader::setAllowMoreThanOne(bool value)
{
    inisettings->setValue("config/AllowMoreThanOneInstance", value);
}
unsigned int PXMIniReader::getUUIDNumber() const
{
    unsigned int i  = 0;
    QString uuidStr = "uuid/";
    while (inisettings->value(uuidStr + QString::number(i), "") == "INUSE") {
        i++;
    }
    if (inisettings->value(uuidStr + QString::number(i), "") == "") {
        inisettings->setValue(uuidStr + QString::number(i), QUuid::createUuid());
    }
    return i;
}
QUuid PXMIniReader::getUUID(unsigned int num, bool takeIt) const
{
    if (inisettings->value("uuid/" + QString::number(num), "").toString().isEmpty()) {
        inisettings->setValue("uuid/" + QString::number(num), QUuid::createUuid().toString());
    }
    QUuid uuid = inisettings->value("uuid/" + QString::number(num), "").toUuid();
    if (takeIt) {
        inisettings->setValue("uuid/" + QString::number(num), "INUSE");
    }
    return uuid;
}
int PXMIniReader::resetUUID(unsigned int num, QUuid uuid)
{
    inisettings->setValue("uuid/" + QString::number(num), uuid.toString());
    return 1;
}
void PXMIniReader::setPort(QString protocol, int portNumber)
{
    inisettings->setValue("port/" + protocol, portNumber);
}
unsigned short PXMIniReader::getPort(QString protocol)
{
    unsigned int portNumber = inisettings->value("port/" + protocol, 0).toUInt();
    if (portNumber == 13649) {
        inisettings->setValue("port/" + protocol, 0);
        portNumber = 0;
    } else if (portNumber != 0 && protocol == QLatin1String("TCP")) {
        portNumber += getUUIDNumber();
    } else if (portNumber >= 65535) {
        qWarning() << "Bad port number in UUID..." << protocol << portNumber;
        portNumber = 0;
    }
    return static_cast<unsigned short>(portNumber);
}
void PXMIniReader::setHostname(QString hostname)
{
    inisettings->setValue("hostname/hostname", hostname.left(PXMConsts::MAX_HOSTNAME_LENGTH));
}
QString PXMIniReader::getHostname(QString defaultHostname)
{
    QString hostname = inisettings->value("hostname/hostname", defaultHostname).toString();
    if (hostname.isEmpty()) {
        inisettings->setValue("hostname/hostname", defaultHostname);
    }

    return hostname.left(PXMConsts::MAX_HOSTNAME_LENGTH).simplified();
}
void PXMIniReader::setWindowSize(QSize windowSize)
{
    if (windowSize.isValid())
        inisettings->setValue("WindowSize/QSize", windowSize);
}
QSize PXMIniReader::getWindowSize(QSize defaultSize) const
{
    QSize windowSize = inisettings->value("WindowSize/QSize", defaultSize).toSize();
    if (windowSize.isValid()) {
        return windowSize;
    } else {
        windowSize = QSize(700, 500);
        inisettings->setValue("WindowSize/QSize", windowSize);
        return windowSize;
    }
}
void PXMIniReader::setMute(bool mute)
{
    inisettings->setValue("config/Mute", mute);
}
bool PXMIniReader::getMute() const
{
    return inisettings->value("config/Mute", false).toBool();
}
void PXMIniReader::setFocus(bool focus)
{
    inisettings->setValue("config/PreventFocus", focus);
}
bool PXMIniReader::getFocus() const
{
    return inisettings->value("config/PreventFocus", false).toBool();
}
QString PXMIniReader::getFont()
{
    return inisettings->value("config/Font", "").toString();
}
void PXMIniReader::setFont(QString font)
{
    inisettings->setValue("config/Font", font);
}
QString PXMIniReader::getMulticastAddress() const
{
    QString ipFull = inisettings->value("net/MulticastAddress", "").toString();
    if (ipFull.isEmpty() || strlen(ipFull.toLatin1().constData()) > INET_ADDRSTRLEN) {
        return QString::fromLocal8Bit(PXMConsts::DEFAULT_MULTICAST_ADDRESS);
    }
    return ipFull;
}
int PXMIniReader::setMulticastAddress(QString ip)
{
    inisettings->setValue("net/MulticastAddress", ip);

    return 0;
}
int PXMIniReader::getVerbosity() const
{
    return inisettings->value("config/DebugVerbosity", 0).toInt();
}

void PXMIniReader::setVerbosity(int level) const
{
    inisettings->setValue("config/DebugVerbosity", level);
}
bool PXMIniReader::getLogActive() const
{
    return inisettings->value("config/LogActive", false).toBool();
}

void PXMIniReader::setLogActive(bool status) const
{
    inisettings->setValue("config/LogActive", status);
}
