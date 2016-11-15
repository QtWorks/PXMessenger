#include <pxmmainwindow.h>

#ifdef _WIN32
Q_DECLARE_METATYPE(intptr_t)
#endif
Q_DECLARE_METATYPE(sockaddr_in)
PXMWindow::PXMWindow(initialSettings presets)
{
#ifdef _WIN32
    qRegisterMetaType<intptr_t>("intptr_t");
#endif
    qRegisterMetaType<sockaddr_in>();
    ourUUIDString = presets.uuid.toString();
    qDebug() << "Our UUID:" << ourUUIDString;

    setupHostname(presets.uuidNum, presets.username);

    if(presets.tcpPort != 0)
        presets.tcpPort += presets.uuidNum;
    if(presets.udpPort == 0)
        presets.udpPort = 13649;
    ourUDPListenerPort = presets.udpPort;

    messServer = new PXMServer(this, presets.tcpPort, presets.udpPort);

    peerWorker = new PXMPeerWorker(this, localHostname, ourUUIDString, messServer);

    setupLayout();

    createTextBrowser();

    createTextEdit();

    createLineEdit();

    createButtons();

    createListWidget();

    createSystemTray();

    createCheckBoxes();

    setupMenuBar();

    setupTooltips();

    createMessServ();

    createMessClient();

    createMessTime();

    connectGuiSignalsAndSlots();

    connectPeerClassSignalsAndSlots();

    this->setCentralWidget(centralwidget);

    focusCheckBox->setChecked(presets.preventFocus);
    muteCheckBox->setChecked(presets.mute);

    statusbar = new QStatusBar(this);
    statusbar->setObjectName(QStringLiteral("statusbar"));
    this->setStatusBar(statusbar);
    this->show();
    this->resize(presets.windowSize);

    QTimer::singleShot(5000, this, SLOT(timerOutSingleShot()));
}
PXMWindow::~PXMWindow()
{
    for(auto &itr : peerWorker->peerDetailsHash)
    {
        if(itr.bev != nullptr)
            bufferevent_free(itr.bev);
        evutil_closesocket(itr.socketDescriptor);
    }
    if(messServer != 0 && messServer->isRunning())
    {
        messServer->requestInterruption();
        messServer->quit();
        messServer->wait(5000);
    }
    messClientThread->quit();
    messClientThread->wait(5000);

    delete messClient;
    delete peerWorker;
    delete [] localHostname;
}

void PXMWindow::setupMenuBar()
{
    menubar = new QMenuBar(this);
    menubar->setObjectName(QStringLiteral("menubar"));
    menubar->setGeometry(QRect(0, 0, 835, 27));
    menubar->setDefaultUp(false);
    this->setMenuBar(menubar);
    QMenu *fileMenu;
    QAction *quitAction = new QAction("&Quit", this);

    fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(quitAction);
    QObject::connect(quitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);

    QMenu *optionsMenu;
    QAction *settingsAction = new QAction("&Settings", this);
    optionsMenu = menuBar()->addMenu("&Options");
    optionsMenu->addAction(settingsAction);
    QObject::connect(settingsAction, &QAction::triggered, this, &PXMWindow::settingsActionsSlot);

    QMenu *helpMenu;
    QAction *aboutAction = new QAction("&About", this);
    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
    QObject::connect(aboutAction, &QAction::triggered, this, &PXMWindow::aboutActionSlot);
}
void PXMWindow::setupTooltips()
{
#ifndef QT_NO_TOOLTIP
    messTextBrowser->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Messages</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messLineEdit->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Hostname</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messSendButton->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Send a Message</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
    messSendButton->setText(QApplication::translate("PXMessenger", "Send", 0));
#ifndef QT_NO_TOOLTIP
    messListWidget->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Connected Peers</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messTextEdit->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Enter message to send</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
    messQuitButton->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Quit PXMessenger</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
    messQuitButton->setText(QApplication::translate("PXMessenger", "Quit", 0));
}
void PXMWindow::setupLayout()
{
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("PXMessenger"));
    this->resize(835, 567);

    centralwidget = new QWidget(this);
    centralwidget->setObjectName(QStringLiteral("centralwidget"));
    layout = new QGridLayout(centralwidget);
    layout->setObjectName(QStringLiteral("layout"));

    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer, 3, 0, 1, 1);
    horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer_3, 5, 2, 1, 1);
    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer_2, 3, 2, 1, 1);
    horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addItem(horizontalSpacer_4, 5, 0, 1, 1);
}
void PXMWindow::createTextEdit()
{
    messTextEdit = new PXMTextEdit(centralwidget);
    messTextEdit->setObjectName(QStringLiteral("messTextEdit"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(messTextEdit->sizePolicy().hasHeightForWidth());
    messTextEdit->setSizePolicy(sizePolicy1);
    messTextEdit->setMaximumSize(QSize(16777215, 150));

    layout->addWidget(messTextEdit, 2, 0, 1, 3);
}
void PXMWindow::createTextBrowser()
{
    messTextBrowser = new QTextBrowser(centralwidget);
    messTextBrowser->setObjectName(QStringLiteral("messTextBrowser"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(1);
    sizePolicy.setVerticalStretch(2);
    sizePolicy.setHeightForWidth(messTextBrowser->sizePolicy().hasHeightForWidth());
    messTextBrowser->setSizePolicy(sizePolicy);
    messTextBrowser->setMinimumSize(QSize(300, 200));

    layout->addWidget(messTextBrowser, 0, 0, 2, 3);
}
void PXMWindow::createLineEdit()
{
    messLineEdit = new QLineEdit(centralwidget);
    messLineEdit->setObjectName(QStringLiteral("messLineEdit"));
    messLineEdit->setMinimumSize(QSize(200, 0));
    messLineEdit->setMaximumSize(QSize(300, 16777215));
    messLineEdit->setText(QString::fromUtf8(localHostname));
    messLineEdit->setReadOnly(true);

    layout->addWidget(messLineEdit, 0, 3, 1, 1);
}
void PXMWindow::createButtons()
{
    messSendButton = new QPushButton(centralwidget);
    messSendButton->setObjectName(QStringLiteral("messSendButton"));
    messSendButton->setMaximumSize(QSize(250, 16777215));
    messSendButton->setLayoutDirection(Qt::LeftToRight);
    messSendButton->setText("Send");

    layout->addWidget(messSendButton, 3, 1, 1, 1);

    messQuitButton = new QPushButton(centralwidget);
    messQuitButton->setObjectName(QStringLiteral("messQuitButton"));
    messQuitButton->setMaximumSize(QSize(250, 16777215));
    messQuitButton->setText("Quit");

    layout->addWidget(messQuitButton, 5, 1, 1, 1);
    messDebugButton = new QPushButton("Debug", this);
    messDebugButton->setGeometry(460, 510, 80, 30);
    connect(messDebugButton, SIGNAL(clicked()), this, SLOT (debugButtonClicked()));
    messDebugButton->hide();
}
void PXMWindow::createListWidget()
{
    globalChatUuid = QUuid::createUuid();

    messListWidget = new QListWidget(centralwidget);
    messListWidget->setObjectName(QStringLiteral("messListWidget"));
    messListWidget->setMinimumSize(QSize(200, 0));
    messListWidget->setMaximumSize(QSize(300, 16777215));

    layout->addWidget(messListWidget, 1, 3, 2, 1);
    //messListWidget->setGeometry(640, 60, 200, 300);
    messListWidget->insertItem(0, "--------------------");
    messListWidget->insertItem(1, "Global Chat");
    messListWidget->item(1)->setData(Qt::UserRole, globalChatUuid);
    messListWidget->item(0)->setFlags(messListWidget->item(0)->flags() & ~Qt::ItemIsEnabled);
}
void PXMWindow::createSystemTray()
{
    QIcon trayIcon(":/resources/resources/70529.png");

    messSystemTrayMenu = new QMenu(this);
    messSystemTrayExitAction = new QAction(tr("&Exit"), this);
    messSystemTrayMenu->addAction(messSystemTrayExitAction);

    messSystemTray = new QSystemTrayIcon(this);
    messSystemTray->setIcon(trayIcon);
    messSystemTray->setContextMenu(messSystemTrayMenu);
    messSystemTray->show();
}
void PXMWindow::createCheckBoxes()
{
    muteCheckBox = new QCheckBox(centralwidget);
    muteCheckBox->setObjectName(QStringLiteral("muteCheckBox"));
    muteCheckBox->setLayoutDirection(Qt::RightToLeft);
    muteCheckBox->setText("Mute");
    layout->addWidget(muteCheckBox, 3, 3, 1, 1);

    focusCheckBox = new QCheckBox(centralwidget);
    focusCheckBox->setObjectName(QStringLiteral("focusCheckBox"));
    focusCheckBox->setLayoutDirection(Qt::RightToLeft);
    focusCheckBox->setText("Prevent focus stealing");

    layout->addWidget(focusCheckBox, 5, 3, 1, 1);
}
void PXMWindow::createMessClient()
{
    messClientThread = new QThread(this);
    messClient = new PXMClient();
    messClient->moveToThread(messClientThread);
    QObject::connect(messClientThread, &QThread::finished, messClientThread, &QObject::deleteLater);
    QObject::connect(messClient, &PXMClient::resultOfConnectionAttempt, peerWorker, &PXMPeerWorker::resultOfConnectionAttempt);
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, peerWorker, &PXMPeerWorker::resultOfTCPSend);
    QObject::connect(this, &PXMWindow::sendMsg, messClient, &PXMClient::sendMsgSlot);
    QObject::connect(this, &PXMWindow::connectToPeer, messClient, &PXMClient::connectToPeer);
    QObject::connect(this, &PXMWindow::sendUDP, messClient, &PXMClient::sendUDP);
    QObject::connect(messServer, &PXMServer::sendMsg, messClient, &PXMClient::sendMsgSlot);
    QObject::connect(messServer, &PXMServer::sendUDP, messClient, &PXMClient::sendUDP);
    messClientThread->start();
}
void PXMWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(messSendButton, &QAbstractButton::clicked, this, &PXMWindow::sendButtonClicked);
    QObject::connect(messQuitButton, &QAbstractButton::clicked, this, &PXMWindow::quitButtonClicked);
    QObject::connect(messListWidget, &QListWidget::currentItemChanged, this, &PXMWindow::currentItemChanged);
    QObject::connect(messTextEdit, &PXMTextEdit::returnPressed, this, &PXMWindow::sendButtonClicked);
    QObject::connect(messSystemTrayExitAction, &QAction::triggered, this, &PXMWindow::quitButtonClicked);
    QObject::connect(messSystemTray, &QSystemTrayIcon::activated, this, &PXMWindow::showWindow);
    QObject::connect(messSystemTray, &QObject::destroyed, messSystemTrayMenu, &QObject::deleteLater);
    QObject::connect(messTextEdit, &QTextEdit::textChanged, this, &PXMWindow::textEditChanged);
    QObject::connect(messSystemTrayMenu, &QMenu::aboutToHide, messSystemTrayMenu, &QObject::deleteLater);;

}
void PXMWindow::connectPeerClassSignalsAndSlots()
{
    QObject::connect(peerWorker, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot);
    QObject::connect(peerWorker, &PXMPeerWorker::connectToPeer, messClient, &PXMClient::connectToPeer);
    QObject::connect(peerWorker, &PXMPeerWorker::printToTextBrowser, this, &PXMWindow::printToTextBrowser);
    QObject::connect(peerWorker, &PXMPeerWorker::setItalicsOnItem, this, &PXMWindow::setItalicsOnItem);
    QObject::connect(peerWorker, &PXMPeerWorker::updateListWidget, this, &PXMWindow::updateListWidget);
}
void PXMWindow::createMessServ()
{
    messServer->setLocalHostname(QString::fromUtf8(localHostname));
    messServer->setLocalUUID(ourUUIDString);
    QObject::connect(messServer, &PXMServer::recievedUUIDForConnection, peerWorker, &PXMPeerWorker::authenticationRecieved);
    QObject::connect(messServer, &PXMServer::messageRecieved, this, &PXMWindow::printToTextBrowserServerSlot );
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &PXMServer::newConnectionRecieved, peerWorker, &PXMPeerWorker::newTcpConnection);
    QObject::connect(messServer, &PXMServer::peerQuit, peerWorker, &PXMPeerWorker::peerQuit);
    QObject::connect(messServer, &PXMServer::attemptConnection, peerWorker, &PXMPeerWorker::attemptConnection);
    QObject::connect(messServer, &PXMServer::sendIps, peerWorker, &PXMPeerWorker::sendIps);
    QObject::connect(messServer, &PXMServer::hostnameCheck, peerWorker, &PXMPeerWorker::hostnameCheck);
    QObject::connect(messServer, &PXMServer::setPeerHostname, peerWorker, &PXMPeerWorker::setPeerHostname);
    QObject::connect(messServer, &PXMServer::setListenerPort, peerWorker, &PXMPeerWorker::setListenerPort);
    QObject::connect(messServer, &PXMServer::setListenerPort, this, &PXMWindow::setListenerPort);
    messServer->start();
}
void PXMWindow::aboutActionSlot()
{
    QMessageBox::about(this, "About", "<br><center>PXMessenger v"
                                      + qApp->applicationVersion() +
                                      "</center>"
                                      "<br>"
                                      "<center>Author: Cory Bolar</center>"
                                      "<br>"
                                      "<center>"
                                      "<a href=\"https://github.com/cbpeckles/PXMessenger\">"
                                      "https://github.com/cbpeckles/PXMessenger</a>"
                                      "</center>"
                                      "<br>");
}
void PXMWindow::setListenerPort(unsigned short port)
{
    ourTCPListenerPort = port;
}
void PXMWindow::settingsActionsSlot()
{
    PXMSettingsDialog *setD = new PXMSettingsDialog(this);
    setD->setupUi();
    setD->readIni();
    setD->show();
}
void PXMWindow::createMessTime()
{
    messTime = time(0);
    currentTime = localtime( &messTime );
}
void PXMWindow::setupHostname(int uuidNum, QString username)
{
    char computerHostname[256];
    localHostname = new char[256 + username.length()];
    memset(localHostname, 0, 1*sizeof(localHostname));
    gethostname(computerHostname, sizeof computerHostname);

    strcat(localHostname, username.toStdString().c_str());
    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        strcat(localHostname, temp);
    }
    strcat(localHostname, "@");
    strcat(localHostname, computerHostname);
}
void PXMWindow::debugButtonClicked()
{

}
QString PXMWindow::getFormattedTime()
{
    char time_str[12];
    messTime = time(0);
    QString time = "";
    time = std::ctime(&messTime);
    time.remove('\n');
    time.push_front('(');
    time.push_back(") ");
    currentTime = localtime(&messTime);
    strftime(time_str, 12, "(%H:%M:%S) ", currentTime);
    return time;
}
void PXMWindow::timerOutRepetitive()
{
    if(messListWidget->count() < 4)
    {
        emit sendUDP("/discover", ourUDPListenerPort);
    }
    else
    {
        qDebug() << "Found enough peers";
        timer->stop();
    }
}
void PXMWindow::timerOutSingleShot()
{
    if(messListWidget->count() < 3)
    {
        QMessageBox::warning(this, "Network Problem", "Could not find anyone, even ourselves, on the network\nThis could indicate a problem with your configuration\n\nWe'll keep looking...");
    }

    if(messListWidget->count() < 4)
    {
        timer = new QTimer(this);
        timer->setInterval(5000);
        QObject::connect(timer, &QTimer::timeout, this, &PXMWindow::timerOutRepetitive);
        emit sendUDP("/discover", ourUDPListenerPort);
        qDebug() << QStringLiteral("Retrying discovery packet, looking for other computers...");
        timer->start();
    }
    else
    {
        qDebug() << "Found enough peers";
    }
}
void PXMWindow::setItalicsOnItem(QUuid uuid, bool italics)
{
    for(int i = 0; i < messListWidget->count() - 2; i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            QFont mfont = messListWidget->item(i)->font();
            mfont.setItalic(italics);
            messListWidget->item(i)->setFont(mfont);
            QString changeInConnection;
            if(italics)
                changeInConnection = " disconnected";
            else
                changeInConnection = " reconnected";
            //this->printToTextBrowser(this->getFormattedTime() % peerWorker->peerDetailsHash[uuid].hostname % " on " % peerWorker->peerDetailsHash[uuid].ipAddress % changeInConnection, uuid, false);
            return;
        }
    }
}
/**
 * @brief 				Stops more than 1000 characters being entered into the QTextEdit object as that is as many as we can send
 */
void PXMWindow::textEditChanged()
{
    if(messTextEdit->toPlainText().length() > 1000)
    {
        int diff = messTextEdit->toPlainText().length() - 1000;
        QString temp = messTextEdit->toPlainText();
        temp.chop(diff);
        messTextEdit->setText(temp);
        QTextCursor cursor(messTextEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        messTextEdit->setTextCursor(cursor);
    }
}
/**
 * @brief 				Signal for out system tray object upon being clicked.  Maximizes the window and brings it into focus
 * @param reason		Qt data type, only care about Trigger
 */
void PXMWindow::showWindow(QSystemTrayIcon::ActivationReason reason)
{
    if( ( ( reason == QSystemTrayIcon::DoubleClick ) | ( reason == QSystemTrayIcon::Trigger ) ) && ! ( this->isVisible() ) )
    {
        this->setWindowState(Qt::WindowMaximized);
        this->show();
        this->setWindowState(Qt::WindowActive);
    }
    return;
}
/**
 * @brief 				Minimize to tray
 * @param event			QT data type, only care about WindowStateChange.  Pass the rest along
 */
void PXMWindow::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange)
    {
        if(isMinimized())
        {
            this->hide();
            event->ignore();
        }
    }
    QWidget::changeEvent(event);
}
/**
 * @brief 				Slot for the QListWidget when the selected item is changed.  We need to change the text in the QTextBrowser, unalert the new item
 * 						and set the scrollbar to be at the bottom of the text browser
 * @param item1			New selected item
 * @param item2			Old selected item.  We do not care about item2 however these are the paramaters of the SIGNAL QListWidget emits
 */
void PXMWindow::currentItemChanged(QListWidgetItem *item1)
{
    int index1 = messListWidget->row(item1);
    QUuid uuid1 = item1->data(Qt::UserRole).toString();
    if(index1 == messListWidget->count() - 1)
    {
        messTextBrowser->setText(globalChat);
    }
    else
    {
        messTextBrowser->setText(peerWorker->peerDetailsHash[uuid1].textBox);
    }
    if(item1->background() == Qt::red)
    {
        this->changeListColor(messListWidget->row(item1), 0);
    }
    QScrollBar *sb = this->messTextBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
    return;
}
/**
 * @brief 				The Quit Debug button was clicked
 */
void PXMWindow::quitButtonClicked()
{
    this->close();
}
void PXMWindow::updateListWidget(QUuid uuid)
{
    messListWidget->setUpdatesEnabled(false);
    int count = messListWidget->count() - 2;
    if(count == 0)
    {
        messListWidget->insertItem(0, peerWorker->peerDetailsHash.value(uuid).hostname);
        messListWidget->item(0)->setData(Qt::UserRole, uuid);
        peerWorker->peerDetailsHash[uuid].listWidgetIndex = 0;

    }
    else
    {
        for(int i = 0; i <= count; i++)
        {
            QUuid u = messListWidget->item(i)->data(Qt::UserRole).toString();
            QString str = messListWidget->item(i)->text();
            if(u == uuid)
            {
                printToTextBrowser(str % " has changed their name to " % peerWorker->peerDetailsHash.value(uuid).hostname, uuid, false);
                messListWidget->item(i)->setText(peerWorker->peerDetailsHash.value(uuid).hostname);
                break;
            }
            if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str, Qt::CaseInsensitive) == 0)
            {
                if(u == uuid)
                    break;
                else
                    continue;
            }
            if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str, Qt::CaseInsensitive) < 0)
            {
                messListWidget->insertItem(i, peerWorker->peerDetailsHash.value((uuid)).hostname);
                messListWidget->item(i)->setData(Qt::UserRole, uuid);
                peerWorker->peerDetailsHash[uuid].listWidgetIndex = i;
                if(!(u.isNull()))
                {
                    peerWorker->peerDetailsHash[u].listWidgetIndex = i+1;
                }
                break;
            }
            else if(peerWorker->peerDetailsHash.value(uuid).hostname.compare(str, Qt::CaseInsensitive) > 0)
            {
                if(i == count)
                {
                    messListWidget->insertItem(i, peerWorker->peerDetailsHash.value(uuid).hostname);
                    messListWidget->item(i)->setData(Qt::UserRole, uuid);
                    peerWorker->peerDetailsHash[uuid].listWidgetIndex = i+1;
                    break;
                }
                else
                    continue;
            }
            else
            {
                qDebug() << "Error Displaying a Peer in MessengerWindow::updateListWidget";
            }
        }
    }
    messListWidget->setUpdatesEnabled(true);
    return;
}
/**
 * @brief				Garbage collection, called upon sending a close signal to the process.  (X button, Quit Debug button, SIGTERM in linux)
 * @param event
 */
void PXMWindow::closeEvent(QCloseEvent *event)
{
    messSystemTray->setContextMenu(NULL);
    messSystemTray->hide();

    MessIniReader iniReader;
    iniReader.setWindowSize(this->size());
    iniReader.setMute(muteCheckBox->isChecked());
    iniReader.setFocus(focusCheckBox->isChecked());
    event->accept();
}
/**
 * @brief 				Send a message to every connected peer
 * @param msg			The message to send
 */
void PXMWindow::globalSend(QString msg)
{
    for(auto &itr : peerWorker->peerDetailsHash)
    {
        if(itr.isConnected)
        {
            emit sendMsg(itr.socketDescriptor, QString::fromUtf8(localHostname) % ": " % msg, "/global", ourUUIDString, "");
        }
    }
    messTextEdit->setText("");
    return;
}
/* Send message button function.  Calls m_client to both connect and send a message to the provided ip_addr*/
void PXMWindow::sendButtonClicked()
{
    if(messListWidget->selectedItems().count() == 0)
    {
        this->printToTextBrowser("Choose a computer to message from the selection pane on the right", "", false);
        return;
    }

    QString str = messTextEdit->toPlainText();
    if(!(str.isEmpty()))
    {
        int index = messListWidget->currentRow();
        QUuid uuidOfSelectedItem = messListWidget->item(index)->data(Qt::UserRole).toString();
        if(uuidOfSelectedItem.isNull())
            return;

        peerDetails destination = peerWorker->peerDetailsHash.value(uuidOfSelectedItem);

        if( ( uuidOfSelectedItem == globalChatUuid) )
        {
            globalSend(str);
            messTextEdit->setText("");
            return;
        }
        if(!(destination.isConnected))
        {
            int s = socket(AF_INET, SOCK_STREAM, 0);
            peerWorker->peerDetailsHash[uuidOfSelectedItem].socketDescriptor = s;
            emit connectToPeer(s, destination.ipAddressRaw);
        }
        emit sendMsg(destination.socketDescriptor, QString::fromUtf8(localHostname) % QStringLiteral(": ") % str, QStringLiteral("/msg"), ourUUIDString, uuidOfSelectedItem.toString());
        messTextEdit->setText("");
    }

    return;
}
void PXMWindow::changeListColor(int row, int style)
{
    QBrush back = messListWidget->item(row)->background();
    if(style == 1)
    {
        messListWidget->item(row)->setBackground(Qt::red);
    }
    if(style == 0)
    {
        messListWidget->item(row)->setBackground(QGuiApplication::palette().base());
    }
    return;
}
/**
 * @brief 				Called when we want to modify the focus or visibility of our program (ie. we get a message)
 */
void PXMWindow::focusWindow()
{
    if(!(muteCheckBox->isChecked()))
        QSound::play(":/resources/resources/message.wav");

    if(!(this->isMinimized()))
    {
        qApp->alert(this, 0);
    }
    else if(this->isMinimized() && !(focusCheckBox->isChecked()))
    {
        this->show();
        qApp->alert(this, 0);
        this->raise();
        this->setWindowState(Qt::WindowActive);
    }
}
/**
 * @brief 					This will print new messages to the appropriate QString and call the focus function if its necessary
 * @param str				Message to print
 * @param peerindex			index of the peers_class->peers array for which the message was meant for
 * @param message			Bool for whether this message that is being printed should alert the listwidgetitem, play sound, and focus the application
 */
void PXMWindow::printToTextBrowser(QString str, QUuid uuid, bool alert)
{
    bool foundIt = false;
    int index = 0;
    for(; index < messListWidget->count(); index++)
    {
        if(messListWidget->item(index)->data(Qt::UserRole) == uuid)
        {
            foundIt = true;
            break;
        }
    }
    if(!foundIt)
        return;

    str = this->getFormattedTime() % str;

    if(uuid == globalChatUuid)
    {
        globalChat.append(str % "\n");
    }
    else
    {
        peerWorker->peerDetailsHash[uuid].textBox.append(str % "\n");
    }

    if(messListWidget->currentItem() != NULL)
    {
        if(uuid == messListWidget->currentItem()->data(Qt::UserRole))
        {
            messTextBrowser->append(str);
            if(alert)
                this->focusWindow();
            return;
        }
    }

    if(alert)
    {
        this->changeListColor(index, 1);
    }
    this->focusWindow();
    return;
}
/**
 * @brief 					This is the slot for a SIGNAL from the mess_serv class when it has recieved a message that it needs to print
 * 							basically this only finds which peer the message is meant for or it if it was mean to be global
 * @param str				The message to print
 * @param ipstr				The ip address of the peer this message has come from
 * @param global			Whether this message should be displayed in the global textbox
 */
void PXMWindow::printToTextBrowserServerSlot(const QString str, QUuid uuid, evutil_socket_t socket, bool global)
{
    /*
    if(peerWorker->peerDetailsHash[uuid].socketDescriptor != socket)
    {
        this->printToTextBrowser("Someone is trying to spoof this users connection, message rejected", uuid, true);
        return;
    }
    */
    if(global)
    {
        uuid = globalChatUuid;
    }
    else if(!(peerWorker->peerDetailsHash.contains(uuid)) && ( uuid != globalChatUuid ))
    {
        qDebug() << "Message from invalid uuid, rejection";
        return;
    }
    this->printToTextBrowser(str, uuid, true);
}
