#include <window.h>

MessengerWindow::MessengerWindow(QUuid uuid, int uuidNum)
{
    ourUUIDString = uuid.toString();
    qDebug() << "Our UUID:" << ourUUIDString;

    //setFixedSize(900,600);

    setupHostname(uuidNum);

    peerWorker = new PeerWorkerClass(this, localHostname, ourUUIDString);

    setupLayout();

    createTextBrowser();

    createTextEdit();

    createLineEdit();

    createButtons();

    createListWidget();

    createSystemTray();

    setupTooltips();

    createMessServ();

    createMessClient();

    createMessTime();

    connectGuiSignalsAndSlots();

    connectPeerClassSignalsAndSlots();

    this->setCentralWidget(centralwidget);
    menubar = new QMenuBar(this);
    menubar->setObjectName(QStringLiteral("menubar"));
    menubar->setGeometry(QRect(0, 0, 835, 27));
    menubar->setDefaultUp(false);
    this->setMenuBar(menubar);
    statusbar = new QStatusBar(this);
    statusbar->setObjectName(QStringLiteral("statusbar"));
    this->setStatusBar(statusbar);
    this->show();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerout()));
    timer->start(5000);

    resize(700, 500);
}
void MessengerWindow::setupTooltips()
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

void MessengerWindow::setupLayout()
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
void MessengerWindow::createTextEdit()
{
    messTextEdit = new MessengerTextEdit(centralwidget);
    messTextEdit->setObjectName(QStringLiteral("messTextEdit"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(messTextEdit->sizePolicy().hasHeightForWidth());
    messTextEdit->setSizePolicy(sizePolicy1);
    messTextEdit->setMaximumSize(QSize(16777215, 150));

    layout->addWidget(messTextEdit, 2, 0, 1, 3);
}
void MessengerWindow::createTextBrowser()
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
void MessengerWindow::createLineEdit()
{
    messLineEdit = new QLineEdit(centralwidget);
    messLineEdit->setObjectName(QStringLiteral("messLineEdit"));
    messLineEdit->setMinimumSize(QSize(200, 0));
    messLineEdit->setMaximumSize(QSize(300, 16777215));
    messLineEdit->setText(QString::fromUtf8(localHostname));
    //messLineEdit->setReadOnly(true);

    layout->addWidget(messLineEdit, 0, 3, 1, 1);
}
void MessengerWindow::createButtons()
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
    //messDebugButton = new QPushButton("Debug", this);
    //messDebugButton->setGeometry(460, 510, 80, 30);
    //connect(messDebugButton, SIGNAL(clicked()), this, SLOT (debugButtonClicked()));
    //messDebugButton->hide();
}
void MessengerWindow::createListWidget()
{
    globalChatUuid = QUuid::createUuid();

    messListWidget = new QListWidget(centralwidget);
    messListWidget->setObjectName(QStringLiteral("messListWidget"));
    messListWidget->setMinimumSize(QSize(200, 0));
    messListWidget->setMaximumSize(QSize(200, 16777215));

    layout->addWidget(messListWidget, 1, 3, 2, 1);
    //messListWidget->setGeometry(640, 60, 200, 300);
    messListWidget->insertItem(0, "--------------------");
    messListWidget->insertItem(1, "Global Chat");
    messListWidget->item(1)->setData(Qt::UserRole, globalChatUuid);
    messListWidget->item(0)->setFlags(messListWidget->item(0)->flags() & ~Qt::ItemIsEnabled);
}
void MessengerWindow::createSystemTray()
{
    messSystemTrayIcon = new QIcon(":/resources/resources/systray.png");

    messSystemTray = new QSystemTrayIcon(this);

    messSystemTrayMenu = new QMenu(this);
    messSystemTrayExitAction = new QAction(messSystemTrayMenu);
    messSystemTrayExitAction = messSystemTrayMenu->addAction("Exit");

    messSystemTray->setContextMenu(messSystemTrayMenu);
    messSystemTray->setIcon(*messSystemTrayIcon);
    messSystemTray->show();
}
void MessengerWindow::createMessClient()
{
    messClientThread = new QThread(this);
    messClient = new MessengerClient();
    messClient->moveToThread(messClientThread);
    messClient->setLocalHostname((localHostname));
    messClient->setlocalUUID(ourUUIDString);
    QObject::connect(messClientThread, SIGNAL(finished()), messClientThread, SLOT(deleteLater()));
    QObject::connect(messClient, SIGNAL (resultOfConnectionAttempt(int, bool)), peerWorker, SLOT(resultOfConnectionAttempt(int,bool)));
    QObject::connect(messClient, SIGNAL (resultOfTCPSend(int, QString, QString, bool)), peerWorker, SLOT(resultOfTCPSend(int,QString,QString,bool)));
    QObject::connect(this, SIGNAL (sendMsg(int, QString, QString, QString, QUuid, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid, QString)));
    QObject::connect(this, SIGNAL (connectToPeer(int, QString, QString)), messClient, SLOT(connectToPeerSlot(int, QString, QString)));
    QObject::connect(messServer, SIGNAL (sendMsg(int, QString, QString, QString, QUuid, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid, QString)));
    QObject::connect(messServer, SIGNAL (sendName(int, QString, QString)), messClient, SLOT (sendNameSlot(int, QString, QString)));
    QObject::connect(messServer, SIGNAL (sendUdp(QString)), messClient, SLOT(udpSendSlot(QString)));
    messClientThread->start();
}
void MessengerWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(messSendButton, SIGNAL (clicked()), this, SLOT (sendButtonClicked()));
    QObject::connect(messQuitButton, SIGNAL (clicked()), this, SLOT (quitButtonClicked()));
    QObject::connect(messListWidget, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT (currentItemChanged(QListWidgetItem*)));
    QObject::connect(messTextEdit, SIGNAL (returnPressed()), this, SLOT (sendButtonClicked()));
    QObject::connect(messSystemTrayExitAction, SIGNAL (triggered()), this, SLOT (quitButtonClicked()));
    QObject::connect(messSystemTray, SIGNAL (activated(QSystemTrayIcon::ActivationReason)), this, SLOT (showWindow(QSystemTrayIcon::ActivationReason)));
    QObject::connect(messSystemTray, SIGNAL(destroyed()), messSystemTrayMenu, SLOT(deleteLater()));
    QObject::connect(messTextEdit, SIGNAL (textChanged()), this, SLOT (textEditChanged()));
    QObject::connect(messSystemTrayMenu, SIGNAL(aboutToHide()), messSystemTrayMenu, SLOT(deleteLater()));;

}
void MessengerWindow::connectPeerClassSignalsAndSlots()
{
    QObject::connect(peerWorker, SIGNAL (sendMsg(int, QString, QString, QString, QUuid, QString)), messClient, SLOT(sendMsgSlot(int, QString, QString, QString, QUuid, QString)));
    QObject::connect(peerWorker, SIGNAL (connectToPeer(int, QString, QString)), messClient, SLOT(connectToPeerSlot(int, QString, QString)));
    QObject::connect(peerWorker, SIGNAL (updateMessServFDS(int)), messServer, SLOT (updateMessServFDSSlot(int)));
    QObject::connect(peerWorker, SIGNAL (printToTextBrowser(QString, QUuid, bool)), this, SLOT (printToTextBrowser(QString, QUuid, bool)));
    QObject::connect(peerWorker, SIGNAL (setItalicsOnItem(QUuid, bool)), this, SLOT (setItalicsOnItem(QUuid, bool)));
    QObject::connect(peerWorker, SIGNAL (updateListWidget(QUuid)), this, SLOT (updateListWidget(QUuid)));
}
void MessengerWindow::createMessServ()
{
    messServer = new MessengerServer(this);
    messServer->setLocalHostname(QString::fromUtf8(localHostname));
    messServer->setLocalUUID(ourUUIDString);
    QObject::connect(messServer, SIGNAL (recievedUUIDForConnection(QString, QString, QString, int, QUuid)), peerWorker, SLOT(updatePeerDetailsHash(QString, QString, QString, int, QUuid)));
    QObject::connect(messServer, SIGNAL (messageRecieved(const QString, QUuid, bool)), this, SLOT (printToTextBrowserServerSlot(const QString, QUuid, bool)) );
    QObject::connect(messServer, SIGNAL (finished()), messServer, SLOT (deleteLater()));
    QObject::connect(messServer, SIGNAL (newConnectionRecieved(int)), peerWorker, SLOT (newTcpConnection(int)));
    QObject::connect(messServer, SIGNAL (peerQuit(int)), peerWorker, SLOT (peerQuit(int)));
    QObject::connect(messServer, SIGNAL (updNameRecieved(QString, QString, QString)), peerWorker, SLOT (attemptConnection(QString, QString, QString)));
    QObject::connect(messServer, SIGNAL (sendIps(int)), peerWorker, SLOT (sendIps(int)));
    QObject::connect(messServer, SIGNAL (hostnameCheck(QString)), peerWorker, SLOT (hostnameCheck(QString)));
    QObject::connect(messServer, SIGNAL (setPeerHostname(QString, QUuid)), peerWorker, SLOT (setPeerHostname(QString, QUuid)));
    QObject::connect(messServer, SIGNAL (setListenerPort(QString)), peerWorker, SLOT (setListenerPort(QString)));
    QObject::connect(this, SIGNAL (retryDiscover()), messServer, SLOT (retryDiscover()));
    messServer->start();
}
void MessengerWindow::createMessTime()
{
    messTime = time(0);
    currentTime = localtime( &messTime );
}
void MessengerWindow::setupHostname(int uuidNum)
{
    char computerHostname[128];
    gethostname(computerHostname, sizeof computerHostname);

#ifdef __unix__
    struct passwd *user;
    user = getpwuid(getuid());
    strcat(localHostname, user->pw_name);
    strcat(localHostname, "@");
    strcat(localHostname, computerHostname);
#elif _WIN32
    char user[UNLEN+1];
    TCHAR t_user[UNLEN+1];
    DWORD user_size = UNLEN+1;
    if(GetUserName(t_user, &user_size))
    {
        wcstombs(user, t_user, UNLEN+1);
        strcat(localHostname, user);
        strcat(localHostname, "@");
    }
    strcat(localHostname, computerHostname);
#endif
    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        strcat(localHostname, temp);
    }
}

void MessengerWindow::debugButtonClicked()
{

}
QString MessengerWindow::getFormattedTime()
{
    char time_str[12];
    messTime = time(0);
    currentTime = localtime(&messTime);
    strftime(time_str, 12, "(%H:%M:%S) ", currentTime);
    return time_str;
}
void MessengerWindow::timerout()
{
    if(messListWidget->count() < 4)
    {
        emit retryDiscover();
        qDebug() << "Retrying Discovery Packet";
    }
    else
    {
        timer->stop();
        qDebug() << "Found enough peers";
    }
}
//Condense the 2 following into one, unsure of how to make the disconnect reconnect feature vary depending on bool
void MessengerWindow::setItalicsOnItem(QUuid uuid, bool italics)
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
            this->printToTextBrowser(this->getFormattedTime() + peerWorker->peerDetailsHash[uuid].hostname + " on " +peerWorker->peerDetailsHash[uuid].ipAddress + changeInConnection, uuid, false);
            return;
        }
    }
}
/**
 * @brief 				Stops more than 1000 characters being entered into the QTextEdit object as that is as many as we can send
 */
void MessengerWindow::textEditChanged()
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
void MessengerWindow::showWindow(QSystemTrayIcon::ActivationReason reason)
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
void MessengerWindow::changeEvent(QEvent *event)
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
 * @brief 				QListWidgetItems in the QListWidget change color when a message has been recieved and not viewed
 * 						This changes them back to default color and resets their text
 * @param item			item to be unalerted
 */
void MessengerWindow::removeMessagePendingStatus(QListWidgetItem* item)
{
    QString temp;

    if(messListWidget->row(item) == messListWidget->count()-1)
    {
        globalChatAlerted = false;
    }
    else
    {
        QUuid temp = (item->data(Qt::UserRole)).toString();
        peerWorker->peerDetailsHash[temp].messagePending = false;
    }
    temp = item->text();
    temp.remove(0, 3);
    temp.remove(temp.length()-3, 3);
    item->setText(temp);

    return;
}
/**
 * @brief 				Slot for the QListWidget when the selected item is changed.  We need to change the text in the QTextBrowser, unalert the new item
 * 						and set the scrollbar to be at the bottom of the text browser
 * @param item1			New selected item
 * @param item2			Old selected item.  We do not care about item2 however these are the paramaters of the SIGNAL QListWidget emits
 */
void MessengerWindow::currentItemChanged(QListWidgetItem *item1)
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
        this->removeMessagePendingStatus(item1);
    }
    QScrollBar *sb = this->messTextBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
    return;
}
/**
 * @brief 				The Quit Debug button was clicked
 */
void MessengerWindow::quitButtonClicked()
{
    this->close();
}
void MessengerWindow::updateListWidget(QUuid uuid)
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
            if((str.startsWith(" * ")))
                str = str.mid(3, str.length()-6);
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
void MessengerWindow::closeEvent(QCloseEvent *event)
{
    if(messServer != 0 && messServer->isRunning())
    {
        messServer->requestInterruption();
        messServer->quit();
        messServer->wait();
    }
    for(auto &itr : peerWorker->peerDetailsHash)
    {
#ifdef __unix__
        ::close(itr.socketDescriptor);
#endif
#ifdef _WIN32
        ::closesocket(itr.socketDescriptor);
#endif
    }
    messClientThread->quit();
    messClientThread->wait();
    timer->stop();
    delete timer;
    delete messClient;
    delete messSystemTrayIcon;
    delete peerWorker;
    messSystemTray->hide();
    event->accept();
}
/**
 * @brief 				Send a message to every connected peer
 * @param msg			The message to send
 */
void MessengerWindow::globalSend(QString msg)
{
    for(auto &itr : peerWorker->peerDetailsHash)
    {
        if(itr.isConnected)
        {
            emit sendMsg(itr.socketDescriptor, msg, localHostname, "/global", ourUUIDString, "");
        }
    }
    messTextEdit->setText("");
    return;
}
/* Send message button function.  Calls m_client to both connect and send a message to the provided ip_addr*/
void MessengerWindow::sendButtonClicked()
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
            emit connectToPeer(s, destination.ipAddress, destination.portNumber);
        }
        emit sendMsg(destination.socketDescriptor, str, localHostname, "/msg", ourUUIDString, uuidOfSelectedItem.toString());
        messTextEdit->setText("");
    }

    return;
}
void MessengerWindow::changeListColor(int row, int style)
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
void MessengerWindow::focusWindow()
{
    if(this->isActiveWindow())
    {
        QSound::play(":/resources/resources/message.wav");
        return;
    }
    else if(this->windowState() == Qt::WindowActive)
    {
        QSound::play(":/resources/resources/message.wav");
        return;
    }
    else if(!(this->isMinimized()))
    {
        QSound::play(":/resources/resources/message.wav");
        qApp->alert(this, 0);
    }
    else if(this->isMinimized())
    {
        QSound::play(":/resources/resources/message.wav");
        this->show();
        qApp->alert(this, 0);
        this->raise();
        this->setWindowState(Qt::WindowActive);
    }
    else
    {
        QSound::play(":/resources/resources/message.wav");
    }
    return;
}
/**
 * @brief 					This will print new messages to the appropriate QString and call the focus function if its necessary
 * @param str				Message to print
 * @param peerindex			index of the peers_class->peers array for which the message was meant for
 * @param message			Bool for whether this message that is being printed should alert the listwidgetitem, play sound, and focus the application
 */
void MessengerWindow::printToTextBrowser(QString str, QUuid uuid, bool message)
{
    QString strnew;

    if(message)
    {
        strnew = this->getFormattedTime() + str;
    }
    else
    {
        strnew = str;
    }

    if(!(peerWorker->peerDetailsHash.contains(uuid)) && ( uuid != globalChatUuid ))
    {
        qDebug() << "Message from invalid uuid, rejection";
        return;
    }

    if(uuid == globalChatUuid)
    {
        globalChat.append(strnew + "\n");
    }
    else
    {
        peerWorker->peerDetailsHash[uuid].textBox.append(strnew + "\n");
    }

    if(messListWidget->currentItem() != NULL)
    {
        if(uuid == messListWidget->currentItem()->data(Qt::UserRole))
        {
            messTextBrowser->append(strnew);
            if(message)
                this->focusWindow();
            return;
        }
    }
    if(message)
    {
        int globalChatIndex = messListWidget->count() - 1;

        if(uuid == globalChatUuid)
        {
            this->changeListColor(globalChatIndex, 1);
            if(!globalChatAlerted)
            {
                messListWidget->item(globalChatIndex)->setText(" * " + messListWidget->item(globalChatIndex)->text() + " * ");
                globalChatAlerted = true;
            }
        }
        else if( !( peerWorker->peerDetailsHash.value(uuid).messagePending ) && ( messListWidget->currentRow() != globalChatIndex ) )
        {
            bool foundIt = false;
            int index = 0;
            int i = 0;
            for(; i < messListWidget->count(); i++)
            {
                if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
                {
                    index = i;
                    foundIt = true;
                    break;
                }
            }
            if(!foundIt)
                return;
            this->changeListColor(index, 1);
            messListWidget->item(index)->setText(" * " + messListWidget->item(index)->text() + " * ");
            peerWorker->peerDetailsHash[uuid].messagePending = true;
        }
        this->focusWindow();
    }
    return;
}
/**
 * @brief 					This is the slot for a SIGNAL from the mess_serv class when it has recieved a message that it needs to print
 * 							basically this only finds which peer the message is meant for or it if it was mean to be global
 * @param str				The message to print
 * @param ipstr				The ip address of the peer this message has come from
 * @param global			Whether this message should be displayed in the global textbox
 */
void MessengerWindow::printToTextBrowserServerSlot(const QString str, QUuid uuid, bool global)
{
    if(global)
    {
        this->printToTextBrowser(str, globalChatUuid, true);
    }
    else
    {
        this->printToTextBrowser(str, uuid, true);
    }
    return;
}
