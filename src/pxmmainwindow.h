#ifndef PXMWINDOW_H
#define PXMWINDOW_H

#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QUuid>

#include "pxmconsts.h"
#include "ui_pxmmainwindow.h"
#include "ui_pxmaboutdialog.h"
#include "ui_pxmsettingsdialog.h"

namespace PXMConsole {
class Window;
}
namespace Ui {
class PXMWindow;
class PXMAboutDialog;
class PXMSettingsDialog;
}
class QListWidgetItem;

class PXMWindow : public QMainWindow
{
    Q_OBJECT

public:
    PXMWindow(QString hostname, QSize windowSize, bool mute, bool focus, QUuid globalChat);
    ~PXMWindow();
    PXMWindow(PXMWindow const&) = delete;
    PXMWindow& operator=(PXMWindow const&) = delete;
    PXMWindow& operator=(PXMWindow&&) noexcept = delete;
    PXMWindow(PXMWindow&&) noexcept = delete;
    PXMConsole::Window *debugWindow = nullptr;
public slots:
    void bloomActionsSlot();
    int printToTextBrowser(QSharedPointer<QString> str, QUuid uuid, bool alert);
    void setItalicsOnItem(QUuid uuid, bool italics);
    void updateListWidget(QUuid uuid, QString hostname);
    void warnBox(QString title, QString msg);
protected:
    void closeEvent(QCloseEvent *event)  Q_DECL_OVERRIDE;
    void changeEvent(QEvent *event)  Q_DECL_OVERRIDE;
private:
    Ui::PXMWindow *ui;
    QAction *messSystemTrayExitAction;
    QMenu *messSystemTrayMenu;
    QSystemTrayIcon *messSystemTray;
    //QLabel *loadingLabel;
    QFrame *fsep;
    QString localHostname;
    QUuid globalChatUuid;

    /*!
     * \brief focusWindow
     *
     * Brings the window to the foreground if allowed by the window manager.
     * Plays a sound when doing this.
     */
    int focusWindow();
    /*!
     * \brief changeListColor
     *
     * Changes the color of the background for an item in a given row of the
     * QListWidget.  Changes to either red or default.
     * \param row Row of item in QListWidget to change
     * \param style Color to change to.  1 for red, 0 for default.
     */
    int changeListItemColor(QUuid uuid, int style);
    /*!
     * \brief createTextBrowser
     *
     * Initializes the text browser where messages are displayed upon sending
     * or receiving.
     */
    void setupLabels();
    /*!
     * \brief createListWidget
     *
     * Initializes the QListWidget that holds the connected computers.  Two
     * items are added, a seperator and a "Global Chat"
     */
    void initListWidget();
    /*!
     * \brief createSystemTray
     *
     * Initializes the icon and menu for a system tray if it is supported.
     */
    void createSystemTray();
    /*!
     * \brief connectGuiSignalsAndSlots
     *
     * All widget slots are connected to the respective signals here
     */
    void connectGuiSignalsAndSlots();
    /*!
     * \brief setupLayout
     *
     * Put all gui widgets on the window and in the correct location.
     * This was created in Qt Designer.
     */
    void setupTooltips();
    void setupMenuBar();
    void setupGui();
    int removeBodyFormatting(QByteArray &str);
private slots:
    int sendButtonClicked();
    void quitButtonClicked();
    void currentItemChanged(QListWidgetItem *item1);
    void textEditChanged();
    void systemTrayAction(QSystemTrayIcon::ActivationReason reason);
    void aboutActionSlot();
    void settingsActionsSlot();
    void debugActionSlot();
    void nameChange(QString hname);
signals:
    void sendMsg(QByteArray, PXMConsts::MESSAGE_TYPE, QUuid);
    void sendUDP(const char*);
    void retryDiscover();
    void addMessageToPeer(QString, QUuid, bool, bool);
};

class PXMAboutDialog : public QDialog
{
    Q_OBJECT

    Ui::PXMAboutDialog *ui;
    QIcon icon;
public:
    PXMAboutDialog(QWidget *parent = 0, QIcon icon = QIcon());
    ~PXMAboutDialog()
    {
        delete ui;
    }
};

class PXMSettingsDialog : public QDialog
{
    Q_OBJECT

    bool AllowMoreThanOneInstance;
    QString hostname;
    int tcpPort;
    int udpPort;
    QFont iniFont;
    int fontSize;
    QString multicastAddress;
    Ui::PXMSettingsDialog *ui;
public:
    PXMSettingsDialog(QWidget *parent = 0);
    void readIni();
    ~PXMSettingsDialog();
private slots:
    void clickedme(QAbstractButton *button);
    void accept();
    void currentFontChanged(QFont font);
    void valueChanged(int size);
signals:
    void nameChange(QString);
    void verbosityChanged();
};

#endif
