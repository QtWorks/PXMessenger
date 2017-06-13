#include "pxmstackwidget.h"

#include <QFile>
#include <QLabel>
#include <QStringBuilder>
#include <QDebug>
#include <QScrollBar>
#include <QDateTime>
#include <QRegularExpression>

using namespace PXMMessageViewer;

StackedWidget::StackedWidget(QWidget* parent) : QStackedWidget(parent)
{
    LabelWidget* lw = new LabelWidget(this, QUuid::createUuid());
    lw->setText("Select a friend on the right to begin chatting!");
    lw->setAlignment(Qt::AlignCenter);
    this->addWidget(lw);
    typingTimer = new QTimer();
    typingTimer->setInterval(typeTimerInterval);
    typingTimer->start();
    QObject::connect(typingTimer, &QTimer::timeout, this, &StackedWidget::timerCallback);
}

void StackedWidget::timerCallback()
{
    qint64 current = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i < this->count(); i++) {
        TextWidget* tw = dynamic_cast<TextWidget*>(this->widget(i));
        if (tw) {
            if (tw->typingTime + 500 < current) {
                // QString temp = tw->info->text();
                tw->timerCallback();
            } else {
                continue;
            }
        }
    }
}

int StackedWidget::newHistory(QUuid& uuid)
{
    if (this->count() < 10) {
        TextWidget* tw = new TextWidget(this, uuid);
        this->addWidget(tw);
    } else {
        History newHist(uuid);
        history.insert(uuid, newHist);
    }
    return 0;
}

TextWidget* StackedWidget::getItem(QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        TextWidget* mvb = dynamic_cast<TextWidget*>(this->widget(i));
        if (mvb && mvb->getIdentifier() == uuid) {
            return mvb;
        }
    }
    return nullptr;
}

int StackedWidget::append(QString str, QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        MVBase* mvb = dynamic_cast<MVBase*>(this->widget(i));
        if (mvb && mvb->getIdentifier() == uuid) {
            TextWidget* tw = qobject_cast<TextWidget*>(this->widget(i));
            if (tw) {
                tw->append(str);
                return 0;
            } else {
                return -1;
            }
        }
    }
    // not in stackwidget
    if (history.value(uuid).getUuid() == uuid) {
        history[uuid].append(str);
    } else {
        return -1;
    }
    return 0;
}

int StackedWidget::removeLastWidget()
{
    if (this->count() < 10) {
        return 0;
    }
    QWidget* qw     = this->widget(this->count() - 1);
    TextWidget* old = qobject_cast<TextWidget*>(qw);
    if (old) {
        History oldHist(old->getIdentifier(), old->toHtml());
        history.insert(oldHist.getUuid(), oldHist);
    } else {
        qWarning() << "Removing non textedit widget";
    }
    this->removeWidget(this->widget(this->count() - 1));

    return 0;
}

int StackedWidget::switchToUuid(QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        MVBase* mvb = dynamic_cast<MVBase*>(this->widget(i));
        if (mvb->getIdentifier() == uuid) {
            QWidget* qw = this->widget(i);
            this->removeWidget(qw);
            this->insertWidget(0, qw);
            this->removeLastWidget();
            this->setCurrentIndex(0);
            return 0;
        }
    }
    // isnt in the list
    if (history.value(uuid).getUuid() == uuid) {
        TextWidget* tw = new TextWidget(this, uuid);
        tw->setHtml(history[uuid].getText());
        this->insertWidget(0, tw);
        this->setCurrentIndex(0);
        this->removeLastWidget();
        history.remove(uuid);
        qDebug() << "Made a new textedit" << history.count() << this->count();
        return 0;
    }
    return -1;
}

void StackedWidget::invert(QUuid uuid)
{
    TextWidget* tw = getItem(uuid);
    if (tw) {
        tw->invert();
    }
}

LabelWidget::LabelWidget(QWidget* parent, const QUuid& uuid) : QLabel(parent), MVBase(uuid)
{
    this->setBackgroundRole(QPalette::Base);
    this->setAutoFillBackground(true);
    this->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
}

void History::append(const QString& str)
{
    text.append(str);
}

void TextWidget::rlabel()
{
    int ax, ay, aw, ah;
    this->frameRect().getRect(&ax, &ay, &aw, &ah);
    info->setGeometry(this->lineWidth() + this->midLineWidth(),
                      ah - info->height() - this->lineWidth() - this->midLineWidth(),
                      aw - 2 * this->lineWidth() - 2 * this->midLineWidth(), info->height());
    QString sheet          = this->styleSheet();
    QRegularExpression qre = QRegularExpression("(QTextBrowser { padding-bottom:)([0-9]*)(.*)");
    if (info->isVisible()) {
        sheet.replace(qre, "\\1" + QString::number(info->height()) + "\\3");

        this->setStyleSheet(sheet);
    } else {
        sheet.replace(qre, "\\10\\3");
        this->setStyleSheet(sheet);
    }
}

void TextWidget::invert()
{
    QString sheet = this->styleSheet();
    QPalette pal  = this->palette();
    QColor col    = pal.base().color();
    int r, g, b;
    col.getRgb(&r, &g, &b);
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
    QRegularExpression qre =
        QRegularExpression("(QTextBrowser {)(.*)(background-color: rgb\\()([0-9]*),([0-9]*),([0-9]*)(.*)");
    sheet.replace(qre, "\\1\\2\\3" + QString::number(r) + "," + QString::number(g) + "," + QString::number(b) + "\\7");
    this->setStyleSheet(sheet);
    sheet = this->styleSheet();
}

TextWidget::TextWidget(QWidget* parent, const QUuid& uuid)
    : QTextBrowser(parent), MVBase(uuid), typing(false), textEntered(false)
{
    this->setOpenExternalLinks(true);
    this->setOpenLinks(true);
    this->setTextInteractionFlags(Qt::TextInteractionFlag::LinksAccessibleByMouse);
    info = new QLineEdit(this);
    info->setStyleSheet(infoBarStyle);
    info->setVisible(false);
    info->setReadOnly(true);
    info->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    QPalette pal = this->palette();
    QColor col   = pal.base().color();
    int r, g, b;
    col.getRgb(&r, &g, &b);
    if (r == 255 && g == 255 && b == 255) {
        pal.setColor(QPalette::Base, pal.alternateBase().color());
        this->setPalette(pal);
        col.getRgb(&r, &g, &b);
        // r = 230;
        // g = 230;
        // b = 230;
    }
    setStyleSheet("QTextBrowser { padding-bottom:0; background-color: rgb(" + QString::number(r) + "," +
                  QString::number(g) + "," + QString::number(b) + ") }");
}

void TextWidget::showTyping(QString hostname)
{
    QScrollBar* scroll = this->verticalScrollBar();
    bool scrollMax     = false;
    if (scroll->sliderPosition() == scroll->maximum()) {
        scrollMax = true;
    }
    info->setText(hostname % infoTyping);
    info->setVisible(true);
    QString sheet          = this->styleSheet();
    QRegularExpression qre = QRegularExpression("(QTextBrowser {)(.*)(padding-bottom:)([0-9]*)(.*)");
    sheet.replace(qre, "\\1\\2\\3" + QString::number(info->height()) + "\\5");

    this->setStyleSheet(sheet);
    if (scrollMax) {
        scroll->setSliderPosition(scroll->maximum());
    }
    // typingTimer->start();
    typing     = true;
    typingTime = QDateTime::currentMSecsSinceEpoch();
}

void TextWidget::showEntered(QString hostname)
{
    this->textEntered = true;
    if (typing) {
        return;
    }
    QScrollBar* scroll = this->verticalScrollBar();
    bool scrollMax     = false;
    if (scroll->sliderPosition() == scroll->maximum()) {
        scrollMax = true;
    }
    info->setText(hostname % infoEntered);
    info->setVisible(true);
    QString sheet          = this->styleSheet();
    QRegularExpression qre = QRegularExpression("(QTextBrowser {)(.*)(padding-bottom:)([0-9]*)(.*)");
    sheet.replace(qre, "\\1\\2\\3" + QString::number(info->height()) + "\\5");

    this->setStyleSheet(sheet);
    if (scrollMax) {
        scroll->setSliderPosition(scroll->maximum());
    }
}

void TextWidget::clearInfoLine()
{
    // typingTimer->stop();
    typing      = false;
    textEntered = false;
    info->setVisible(false);
    QString sheet          = this->styleSheet();
    QRegularExpression qre = QRegularExpression("(QTextBrowser {)(.*)(padding-bottom:)([0-9]*)(.*)");
    sheet.replace(qre, "\\1\\2\\30\\5");

    this->setStyleSheet(sheet);
}

void TextWidget::timerCallback()
{
    if (textEntered) {
        typing       = false;
        QString temp = info->text();
        if (temp.endsWith(infoTyping)) {
            this->showEntered(temp.left(temp.length() - infoTyping.length()));
        }
    } else {
        clearInfoLine();
    }
}

int PXMMessageViewer::StackedWidget::showTyping(QUuid& uuid, QString hostname)
{
    for (int i = 0; i < this->count(); i++) {
        TextWidget* tw = dynamic_cast<TextWidget*>(this->widget(i));
        if (tw && tw->getIdentifier() == uuid) {
            tw->showTyping(hostname);
            return 0;
        }
    }
    return 1;
}

int StackedWidget::showEntered(QUuid& uuid, QString hostname)
{
    for (int i = 0; i < this->count(); i++) {
        TextWidget* tw = dynamic_cast<TextWidget*>(this->widget(i));
        if (tw && tw->getIdentifier() == uuid) {
            tw->showEntered(hostname);
            return 0;
        }
    }
    return 1;
}
int StackedWidget::clearInfoLine(QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        TextWidget* tw = dynamic_cast<TextWidget*>(this->widget(i));
        if (tw && tw->getIdentifier() == uuid) {
            tw->clearInfoLine();
            return 0;
        }
    }
    return 1;
}
