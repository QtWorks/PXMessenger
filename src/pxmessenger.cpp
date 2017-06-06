#include <QApplication>
#include <QDebug>
#include <QStringBuilder>
#include <QDateTime>
#include <QTimer>
#include <QStyleFactory>

#include "pxmagent.h"
#include "pxmconsole.h"

#ifdef _WIN32
QLatin1Char seperator = QLatin1Char('\\');
#define WIN32_LEAN_AND_MEAN
#else
QLatin1Char seperator = QLatin1Char('/');
#endif

#ifdef QT_DEBUG
void debugMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
#else
void debugMessageOutput(QtMsgType type, const QMessageLogContext&, const QString& msg)
#endif
{
    using namespace PXMConsole;
    Logger* logger = Logger::getInstance();

    switch (logger->getVerbosityLevel()) {
        case 0:
            if (type == QtDebugMsg || type == QtWarningMsg)
                return;
            break;
        case 1:
            if (type == QtDebugMsg)
                return;
            break;
        default:
            break;
    }

    QByteArray localMsg = QByteArray();
    localMsg.append(QDateTime::currentDateTime().time().toString(QStringLiteral("[hh:mm:ss] ")));

    QColor msgColor;
    switch (type) {
        case QtDebugMsg:
            localMsg.append(QString("%1").arg(QStringLiteral("DEBUG:"), -7, QChar(' ')));
            msgColor = Qt::gray;
            break;
        case QtWarningMsg:
            localMsg.append(QString("%1").arg(QStringLiteral("WARN:"), -7, QChar(' ')));
            msgColor = Qt::darkYellow;
            break;
        case QtCriticalMsg:
            localMsg.append(QString("%1").arg(QStringLiteral("CRIT:"), -7, QChar(' ')));
            msgColor = Qt::red;
            break;
        case QtFatalMsg:
            localMsg.append(QString("%1").arg(QStringLiteral("FATAL:"), -7, QChar(' ')));
            abort();
            break;
        case QtInfoMsg:
            localMsg.append(QString("%1").arg(QStringLiteral("INFO:"), -7, QChar(' ')));
            msgColor = QGuiApplication::palette().foreground().color();
            break;
    }

    QString filename = QString();
#ifdef QT_DEBUG
    filename = QString::fromUtf8(context.file);
    filename = filename.right(filename.length() - filename.lastIndexOf(seperator) - 1);
    filename = QString("%1").arg(filename.append(':' + QString::number(context.line)), -((int)PXMConsts::DEBUG_PADDING),
                                 QChar(' '));
#endif /* end QT_DEBUG */
    localMsg.append(filename.toUtf8() % msg.toLatin1());

    localMsg.append(QChar('\n'));
    const char* cmsg = localMsg.constData();
    fprintf(stderr, "%s", cmsg);
    if (Window::textEdit) {
        qApp->postEvent(logger, new AppendTextEvent(localMsg, msgColor), Qt::LowEventPriority);
    }
    if (logger->getLogStatus() && logger->logFile->isOpen()) {
        logger->logFile->write(cmsg, strlen(cmsg));
        logger->logFile->flush();
    }
}

void testPalette()
{
    qApp->setStyle(QStyleFactory::create("fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15, 15, 15));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);

    palette.setColor(QPalette::Highlight, QColor(142, 45, 197).lighter());
    palette.setColor(QPalette::HighlightedText, Qt::black);
    qApp->setPalette(palette);
}

int main(int argc, char** argv)
{
    qInstallMessageHandler(debugMessageOutput);

    QApplication app(argc, argv);
    QObject::connect(&app, &QApplication::lastWindowClosed, &app, &QApplication::quit);

    app.setApplicationName("PXMessenger");
    app.setOrganizationName("PXMessenger");
    app.setOrganizationDomain("PXMessenger");
    app.setApplicationVersion("1.6.0");

    int result;
    {
        PXMAgent overlord;

        QMetaObject::invokeMethod(&overlord, "init", Qt::QueuedConnection);
        QObject::connect(&overlord, &PXMAgent::alreadyRunning, &app, &QApplication::quit);

        result = app.exec();

        qInfo().noquote() << QStringLiteral("Exiting PXMessenger");
    }
    qInfo().noquote() << QStringLiteral("Successful Shutdown with code:") << result;

    return result;
}
