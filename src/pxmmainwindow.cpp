#include <pxmmainwindow.h>

PXMWindow::PXMWindow(initialSettings presets)
{
    //Init
    localHostname = QString();
    globalChatAlerted = false;
    multicastFunctioning = false;
    debugWindow = nullptr;
    globalChatUuid = QUuid::createUuid();
    //End of Init

    debugWindow = new PXMDebugWindow();

    ourUUID = presets.uuid.toString();

    qDebug() << "Our UUID:" << ourUUID.toString();

    setupHostname(presets.uuidNum, presets.username);

    if(presets.tcpPort != 0)
        presets.tcpPort += presets.uuidNum;
    if(presets.udpPort == 0)
        presets.udpPort = DEFAULT_UDP_PORT;
    ourUDPListenerPort = presets.udpPort;

    if(presets.multicast.isEmpty() || strlen(presets.multicast.toLatin1().constData()) > INET_ADDRSTRLEN)
        presets.multicast = QString::fromLocal8Bit(MULTICAST_ADDRESS);

    in_addr multicast_in_addr;
    multicast_in_addr.s_addr = inet_addr(presets.multicast.toLatin1().constData());
    //inet_pton(AF_INET,presets.multicast.toLatin1().constData(), &multicast_in_addr);

    messServer = new PXMServer(this, presets.tcpPort, presets.udpPort, multicast_in_addr);

    peerWorker = new PXMPeerWorker(nullptr, localHostname, ourUUID, presets.multicast, messServer, globalChatUuid);

    messClient = new PXMClient(multicast_in_addr);

    startWorkerThread();

    startServerThread();

    connectPeerClassSignalsAndSlots();

    setupGui();

    focusCheckBox->setChecked(presets.preventFocus);
    muteCheckBox->setChecked(presets.mute);

    this->resize(presets.windowSize);

    this->presets = presets;

}
PXMWindow::~PXMWindow()
{
    peerWorker->disconnect(messServer);

    if(workerThread != 0 && workerThread->isRunning())
    {
        workerThread->quit();
        workerThread->wait(5000);
    }
    qDebug() << "Shutdown of WorkerThread Successful";

    if(messServer != 0 && messServer->isRunning())
    {
        bufferevent_write(closeBev, "/exit", 5);
        messServer->wait(5000);
    }
    qDebug() << "Shutdown of PXMServer Successful";

    qDeleteAll(globalChat);

    delete debugWindow;

    qDebug() << "Shutdown of PXMWindow Successful";
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
    QAction *bloomAction = new QAction("&Bloom", this);
    optionsMenu = menuBar()->addMenu("&Tools");
    optionsMenu->addAction(settingsAction);
    optionsMenu->addAction(bloomAction);
    QObject::connect(settingsAction, &QAction::triggered, this, &PXMWindow::settingsActionsSlot);
    QObject::connect(bloomAction, &QAction::triggered, this, &PXMWindow::bloomActionsSlot);

    QMenu *helpMenu;
    QAction *aboutAction = new QAction("&About", this);
    QAction *debugAction = new QAction("&Debug", this);
    helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(debugAction);
    QObject::connect(aboutAction, &QAction::triggered, this, &PXMWindow::aboutActionSlot);
    QObject::connect(debugAction, &QAction::triggered, this, &PXMWindow::debugActionSlot);
}
void PXMWindow::setupTooltips()
{
#ifndef QT_NO_TOOLTIP
    messSendButton->setToolTip(QApplication::translate("PXMessenger", "<html><head/><body><p>Send a Message</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
    messSendButton->setText(QApplication::translate("PXMessenger", "Send", 0));
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
    messTextEdit->setPlaceholderText(QStringLiteral("Enter a message to send here!"));

    layout->addWidget(messTextEdit, 2, 0, 1, 3);
}
void PXMWindow::createTextBrowser()
{
    messTextBrowser = new PXMTextBrowser(centralwidget);
    messTextBrowser->setObjectName(QStringLiteral("messTextBrowser"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(1);
    sizePolicy.setVerticalStretch(2);
    sizePolicy.setHeightForWidth(messTextBrowser->sizePolicy().hasHeightForWidth());
    messTextBrowser->setSizePolicy(sizePolicy);
    messTextBrowser->setMinimumSize(QSize(300, 200));

    layout->addWidget(messTextBrowser, 0, 0, 2, 3);

    loadingLabel = new QLabel(centralwidget);
    loadingLabel->setText("Select a Friend on the right to begin messaging!");
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setGeometry(messTextBrowser->geometry());
    loadingLabel->show();
    resizeLabel(messTextBrowser->geometry());

    /*
    browserLabel = new QLabel(messTextBrowser);
    browserLabel->setText("hello");
    browserLabel->setAlignment(Qt::AlignCenter);
    browserLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    QRect r = messTextBrowser->contentsRect();
    QFontMetrics fm = QFontMetrics(qApp->font());
    r.setHeight(fm.height()+fm.lineSpacing());
    r.adjust(1,1,-1,-1);

    browserLabel->setGeometry(r);
    browserLabel->show();
    */
}
void PXMWindow::resizeLabel(QRect size)
{
    if((loadingLabel))
    {
        loadingLabel->setGeometry(size);
        //loadingLabel->setStyleSheet("border: 2px; border-style: solid;");
    }
}

void PXMWindow::createLineEdit()
{
    messLineEdit = new QLineEdit(centralwidget);
    messLineEdit->setObjectName(QStringLiteral("messLineEdit"));
    messLineEdit->setMinimumSize(QSize(200, 0));
    messLineEdit->setMaximumSize(QSize(300, 16777215));
    messLineEdit->setText(localHostname);
    messLineEdit->setReadOnly(true);

    layout->addWidget(messLineEdit, 0, 3, 1, 1);
}
void PXMWindow::createButtons()
{
    messSendButton = new QPushButton(centralwidget);
    messSendButton->setObjectName(QStringLiteral("messSendButton"));
    messSendButton->setMaximumSize(QSize(250, 16777215));
    messSendButton->setLayoutDirection(Qt::LeftToRight);
    messSendButton->setText(QStringLiteral("Send"));

    layout->addWidget(messSendButton, 3, 1, 1, 1);

    messQuitButton = new QPushButton(centralwidget);
    messQuitButton->setObjectName(QStringLiteral("messQuitButton"));
    messQuitButton->setMaximumSize(QSize(250, 16777215));
    messQuitButton->setText(QStringLiteral("Quit"));

    layout->addWidget(messQuitButton, 5, 1, 1, 1);
}
void PXMWindow::createListWidget()
{

    messListWidget = new QListWidget(centralwidget);
    messListWidget->setObjectName(QStringLiteral("messListWidget"));
    messListWidget->setMinimumSize(QSize(200, 0));
    messListWidget->setMaximumSize(QSize(300, 16777215));

    layout->addWidget(messListWidget, 1, 3, 2, 1);
    messListWidget->insertItem(0, QStringLiteral("Global Chat"));
    QListWidgetItem *seperator = new QListWidgetItem(messListWidget);
    seperator->setSizeHint(QSize(200, 10));
    seperator->setFlags(Qt::NoItemFlags);
    messListWidget->insertItem(1, seperator);
    fsep = new QFrame(messListWidget);
    fsep->setFrameStyle(QFrame::HLine | QFrame::Plain);
    fsep->setLineWidth(2);
    messListWidget->setItemWidget(seperator, fsep);
    messListWidget->item(0)->setData(Qt::UserRole, globalChatUuid);
    messListWidget->setSortingEnabled(false);
}
void PXMWindow::createSystemTray()
{
    QIcon trayIcon(":/resources/resources/70529.png");

    messSystemTrayMenu = new QMenu(this);
    messSystemTrayExitAction = new QAction(tr("&Exit"), this);
    messSystemTrayMenu->addAction(messSystemTrayExitAction);
    QObject::connect(messSystemTrayExitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

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
    muteCheckBox->setText(QStringLiteral("Mute"));
    layout->addWidget(muteCheckBox, 3, 3, 1, 1);

    focusCheckBox = new QCheckBox(centralwidget);
    focusCheckBox->setObjectName(QStringLiteral("focusCheckBox"));
    focusCheckBox->setLayoutDirection(Qt::RightToLeft);
    focusCheckBox->setText(QStringLiteral("Prevent focus stealing"));

    layout->addWidget(focusCheckBox, 5, 3, 1, 1);
}

void PXMWindow::setupGui()
{
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

    connectGuiSignalsAndSlots();

    this->setCentralWidget(centralwidget);

    this->show();
}
void PXMWindow::startWorkerThread()
{
    workerThread = new QThread(this);
    workerThread->setObjectName("WorkerThread");
    messClient->moveToThread(workerThread);
    peerWorker->moveToThread(workerThread);
    QObject::connect(workerThread, &QThread::started, peerWorker, &PXMPeerWorker::currentThreadInit);
    QObject::connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    QObject::connect(workerThread, &QThread::finished, peerWorker, &QObject::deleteLater);
    QObject::connect(workerThread, &QThread::finished, messClient, &QObject::deleteLater);
    QObject::connect(messClient, &PXMClient::resultOfConnectionAttempt, peerWorker, &PXMPeerWorker::resultOfConnectionAttempt, Qt::AutoConnection);
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, peerWorker, &PXMPeerWorker::resultOfTCPSend, Qt::AutoConnection);
    QObject::connect(this, &PXMWindow::sendMsg, peerWorker, &PXMPeerWorker::sendMsgAccessor, Qt::QueuedConnection);
    QObject::connect(this, &PXMWindow::sendUDP, messClient, &PXMClient::sendUDP, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::sendUDP, messClient, &PXMClient::sendUDP, Qt::QueuedConnection);
    workerThread->start();
}
void PXMWindow::connectGuiSignalsAndSlots()
{
    QObject::connect(messSendButton, &QAbstractButton::clicked, this, &PXMWindow::sendButtonClicked);
    QObject::connect(messQuitButton, &QAbstractButton::clicked, this, &PXMWindow::quitButtonClicked);
    QObject::connect(messListWidget, &QListWidget::currentItemChanged, this, &PXMWindow::currentItemChanged);
    QObject::connect(messTextEdit, &PXMTextEdit::returnPressed, this, &PXMWindow::sendButtonClicked);
    QObject::connect(messSystemTray, &QSystemTrayIcon::activated, this, &PXMWindow::showWindow);
    QObject::connect(messSystemTray, &QObject::destroyed, messSystemTrayMenu, &QObject::deleteLater);
    QObject::connect(messTextEdit, &QTextEdit::textChanged, this, &PXMWindow::textEditChanged);
    QObject::connect(messSystemTrayMenu, &QMenu::aboutToHide, messSystemTrayMenu, &QObject::deleteLater);;
    QObject::connect(messTextBrowser, &PXMTextBrowser::resizeLabel, this, &PXMWindow::resizeLabel);
}
void PXMWindow::connectPeerClassSignalsAndSlots()
{
    QObject::connect(peerWorker, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot, Qt::AutoConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::connectToPeer, messClient, &PXMClient::connectToPeer, Qt::AutoConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::printToTextBrowser, this, &PXMWindow::printToTextBrowser, Qt::QueuedConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::setItalicsOnItem, this, &PXMWindow::setItalicsOnItem, Qt::QueuedConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::updateListWidget, this, &PXMWindow::updateListWidget, Qt::QueuedConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::sendIpsPacket, messClient, &PXMClient::sendIpsSlot, Qt::AutoConnection);
    QObject::connect(peerWorker, &PXMPeerWorker::warnBox, this, &PXMWindow::warnBox, Qt::AutoConnection);
    QObject::connect(this, &PXMWindow::addMessageToPeer, peerWorker, &PXMPeerWorker::addMessageToPeer, Qt::QueuedConnection);
    //QObject::connect(this, &PXMWindow::addMessageToAllPeers, peerWorker, &PXMPeerWorker::addMessageToAllPeers, Qt::QueuedConnection);
    QObject::connect(this, &PXMWindow::printFullHistory, peerWorker, &PXMPeerWorker::printFullHistory, Qt::QueuedConnection);
    QObject::connect(debugWindow->pushButton, &QAbstractButton::clicked, peerWorker, &PXMPeerWorker::printInfoToDebug);
    QObject::connect(messServer, &PXMServer::multicastIsFunctional, peerWorker, &PXMPeerWorker::multicastIsFunctional, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::serverSetupFailure, peerWorker, &PXMPeerWorker::serverSetupFailure, Qt::QueuedConnection);
}
void PXMWindow::startServerThread()
{
    messServer->setLocalHostname(localHostname);
    messServer->setLocalUUID(ourUUID);
    QObject::connect(messServer, &PXMServer::authenticationReceived, peerWorker, &PXMPeerWorker::authenticationReceived, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::messageRecieved, peerWorker, &PXMPeerWorker::recieveServerMessage, Qt::QueuedConnection);
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &PXMServer::newTCPConnection, peerWorker, &PXMPeerWorker::newTcpConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::peerQuit, peerWorker, &PXMPeerWorker::peerQuit, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::attemptConnection, peerWorker, &PXMPeerWorker::attemptConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::sendSyncPacket, peerWorker, &PXMPeerWorker::sendSyncPacketBev, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::syncPacketIterator, peerWorker, &PXMPeerWorker::syncPacketIterator, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setPeerHostname, peerWorker, &PXMPeerWorker::setPeerHostname, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setListenerPorts, peerWorker, &PXMPeerWorker::setListenerPorts, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::libeventBackend, peerWorker, &PXMPeerWorker::setlibeventBackend, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setCloseBufferevent, this, &PXMWindow::setCloseBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setSelfCommsBufferevent, peerWorker, &PXMPeerWorker::setSelfCommsBufferevent, Qt::QueuedConnection);
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
void PXMWindow::settingsActionsSlot()
{
    PXMSettingsDialog *setD = new PXMSettingsDialog(this);
    setD->setupUi();
    setD->readIni();
    setD->show();
}
void PXMWindow::bloomActionsSlot()
{
    QMessageBox box;
    box.setText("This will resend our initial discovery "
                "packet to the multicast group.  If we "
                "have only found ourselves this is happening "
                "automatically on a 15 second timer.");
    QPushButton *bloomButton = box.addButton(tr("Bloom"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Abort);
    box.exec();
    if(box.clickedButton() == bloomButton)
    {
        qDebug() << "User triggered bloom";
        emit sendUDP("/discover", ourUDPListenerPort);
    }
}
void PXMWindow::setCloseBufferevent(bufferevent *bev)
{
    closeBev = bev;
}
void PXMWindow::warnBox(QString title, QString msg)
{
    QMessageBox::warning(this, title, msg);
}
void PXMWindow::setupHostname(int uuidNum, QString username)
{
    char computerHostname[256];

    gethostname(computerHostname, sizeof computerHostname);

    localHostname.append(username);
    if(uuidNum > 0)
    {
        char temp[3];
        sprintf(temp, "%d", uuidNum);
        localHostname.append(QString::fromUtf8(temp));
    }
    localHostname.append("@");
    localHostname.append(QString::fromLocal8Bit(computerHostname).left(MAX_HOSTNAME_LENGTH));
}
void PXMWindow::debugActionSlot()
{
    if(debugWindow)
    {
        debugWindow->show();
        debugWindow->setWindowState(Qt::WindowActive);
    }
}

void PXMWindow::setItalicsOnItem(QUuid uuid, bool italics)
{
    for(int i = 0; i < messListWidget->count(); i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            QFont mfont = messListWidget->item(i)->font();
            if(mfont.italic() == italics)
                return;
            mfont.setItalic(italics);
            messListWidget->item(i)->setFont(mfont);
            QString changeInConnection;
            if(italics)
                changeInConnection = " disconnected";
            else
                changeInConnection = " reconnected";
            emit addMessageToPeer(messListWidget->item(i)->text() % changeInConnection, uuid, false, true);
            return;
        }
    }
}
void PXMWindow::textEditChanged()
{
    if(messTextEdit->toPlainText().length() > TEXT_EDIT_MAX_LENGTH)
    {
        int diff = messTextEdit->toPlainText().length() - TEXT_EDIT_MAX_LENGTH;
        QString temp = messTextEdit->toPlainText();
        temp.chop(diff);
        messTextEdit->setText(temp);
        QTextCursor cursor(messTextEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        messTextEdit->setTextCursor(cursor);
    }
}
void PXMWindow::showWindow(QSystemTrayIcon::ActivationReason reason)
{
    if( ( ( reason == QSystemTrayIcon::DoubleClick ) || ( reason == QSystemTrayIcon::Trigger ) ) && ! ( this->isVisible() ) )
    {
        this->setWindowState(Qt::WindowMaximized);
        this->show();
        this->setWindowState(Qt::WindowActive);
    }
    return;
}
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
void PXMWindow::currentItemChanged(QListWidgetItem *item1)
{
    loadingLabel->setText("Loading history...");
    messTextBrowser->clear();
    QUuid uuid = item1->data(Qt::UserRole).toString();
    loadingLabel->show();
    emit printFullHistory(uuid);
    if(item1->background() == Qt::red)
    {
        this->changeListItemColor(uuid, 0);
    }
    QScrollBar *sb = this->messTextBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
    return;
}
void PXMWindow::quitButtonClicked()
{
    qApp->quit();
}
void PXMWindow::updateListWidget(QUuid uuid, QString hostname)
{
    messListWidget->setUpdatesEnabled(false);
    for(int i = 2; i < messListWidget->count(); i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole).toUuid() == uuid)
        {
            if(messListWidget->item(i)->text() != hostname)
            {
                emit addMessageToPeer(messListWidget->item(i)->text() % " has changed their name to " % hostname, uuid, false, false);
                messListWidget->item(i)->setText(hostname);
                QListWidgetItem *global = messListWidget->takeItem(0);
                messListWidget->sortItems();
                messListWidget->insertItem(0, global);
            }
            setItalicsOnItem(uuid, 0);
            messListWidget->setUpdatesEnabled(true);
            return;
        }
    }

    QListWidgetItem *global = messListWidget->takeItem(0);
    QListWidgetItem *item = new QListWidgetItem(hostname, messListWidget);

    item->setData(Qt::UserRole, uuid);
    messListWidget->addItem(item);
    messListWidget->sortItems();
    messListWidget->insertItem(0, global);

    messListWidget->setUpdatesEnabled(true);
}
void PXMWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "starting closeEvent";
    qDebug() << "trying to hide systray";
    messSystemTray->hide();
    qDebug() << "systray hidden";
    MessIniReader iniReader;
    qDebug() << "creating iniReader";
    iniReader.setWindowSize(this->size());
    iniReader.setMute(muteCheckBox->isChecked());
    iniReader.setFocus(focusCheckBox->isChecked());
    qDebug() << "ini done";

    qDebug() << "closeEvent calling event->accept()";
    debugWindow->hide();
    event->accept();
}
int PXMWindow::sendButtonClicked()
{
    if(!messListWidget->currentItem())
    {
        return -1;
    }
    QByteArray msg = messTextEdit->toPlainText().toUtf8();
    //QByteArray msg = messTextEdit->toHtml().toUtf8();
    //int len1 = msg.length();
    if(!(msg.isEmpty()))
    {
        int index = messListWidget->currentRow();
        QUuid uuidOfSelectedItem = messListWidget->item(index)->data(Qt::UserRole).toString();

        if(uuidOfSelectedItem.isNull())
            return -1;

        if( ( uuidOfSelectedItem == globalChatUuid) )
        {
            emit sendMsg(msg, "/global", ourUUID, QUuid());
        }
        else
        {
            emit sendMsg(msg, "/msg", ourUUID, uuidOfSelectedItem);
        }
        messTextEdit->setText("");
    }
    else
    {
        return -1;
    }
    return 0;
}
int PXMWindow::changeListItemColor(QUuid uuid, int style)
{
    for(int i = 0; i < messListWidget->count(); i++)
    {
        if(messListWidget->item(i)->data(Qt::UserRole) == uuid)
        {
            if(!style)
                messListWidget->item(i)->setBackground(QGuiApplication::palette().base());
            else
                messListWidget->item(i)->setBackground(QBrush(QColor(Qt::red)));
            break;
        }
    }
    return 0;
}
int PXMWindow::focusWindow()
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
    return 0;
}
int PXMWindow::printToTextBrowser(QString str, QUuid uuid, bool alert)
{
    if(alert)
    {
        if(messListWidget->currentItem())
        {
            if(messListWidget->currentItem()->data(Qt::UserRole) != uuid)
            {
                changeListItemColor(uuid, 1);
                this->focusWindow();
            }
        }
        else
        {
            changeListItemColor(uuid, 1);
            this->focusWindow();
        }
    }
    if(!messListWidget->currentItem() || messListWidget->currentItem()->data(Qt::UserRole) != uuid)
    {
        return -1;
    }

    messTextBrowser->setUpdatesEnabled(false);
    messTextBrowser->append(str);
    if(loadingLabel)
        loadingLabel->hide();
    messTextBrowser->setUpdatesEnabled(true);

    return 0;
}
