#ifndef PXMTEXTEDIT_H
#define PXMTEXTEDIT_H

#include <QTextEdit>
#include <QKeyEvent>
#include <QWidget>

class PXMTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit PXMTextEdit(QWidget* parent);
    void keyPressEvent(QKeyEvent *event);
signals:
    void returnPressed();
protected:
    void focusOutEvent(QFocusEvent *e);
    void focusInEvent(QFocusEvent *e);
};

#endif // MESS_TEXTEDIT_H
