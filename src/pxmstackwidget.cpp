#include "pxmstackwidget.h"
#include <QFile>
#include <QLabel>
#include <QStringBuilder>
#include <QDebug>

using namespace PXMMessageViewer;

StackedWidget::StackedWidget(QWidget* parent) : QStackedWidget(parent)
{
    LabelWidget* lw = new LabelWidget(this, QUuid::createUuid());
    lw->setText("Select a friend on the right to begin chatting!");
    lw->setAlignment(Qt::AlignCenter);
    this->addWidget(lw);
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
