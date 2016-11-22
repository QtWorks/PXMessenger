#ifndef MESSINIREADER_H
#define MESSINIREADER_H

#include <QString>
#include <QDebug>
#include <QUuid>
#include <QSettings>
#include <QSize>

class MessIniReader
{
public:
    MessIniReader();
    ~MessIniReader();
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
private:
    QSettings *inisettings;
};

#endif // MESSINIREADER_H
