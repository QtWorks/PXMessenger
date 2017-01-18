#ifndef MESSINIREADER_H
#define MESSINIREADER_H

#include "pxmconsts.h"

class QSettings;
class QString;
class QSize;
class PXMIniReader
{
    QSettings *inisettings;
public:
    PXMIniReader();
    ~PXMIniReader();
    bool checkAllowMoreThanOne();
    int getUUIDNumber();
    QUuid getUUID(int num, bool takeIt);
    int resetUUID(int num, QUuid uuid);
    unsigned short getPort(QString protocol);
    QString getHostname(QString defaultHostname);
    void setHostname(QString hostname);
    void setPort(QString protocol, int portNumber);
    void setWindowSize(QSize windowSize);
    QSize getWindowSize(QSize defaultSize);
    void setMute(bool mute);
    bool getMute();
    void setFocus(bool focus);
    bool getFocus();
    QString getFont();
    void setFont(QString font);
    int getFontSize();
    void setFontSize(int size);
    void setAllowMoreThanOne(bool value);
    int setMulticastAddress(QString ip);
    QString getMulticastAddress();
    int getVerbosity();
    void setVerbosity(int level);
};

#endif // MESSINIREADER_H
