#ifndef PXMAGENT_H
#define PXMAGENT_H

#include <QObject>
#include <QScopedPointer>
#include <QPalette>

#include <pxmmainwindow.h>
#include <pxmpeerworker.h>

class PXMAgentPrivate;
class PXMAgent : public QObject
{
    Q_OBJECT

    QScopedPointer<PXMAgentPrivate> d_ptr;

   public:
    // Default
    PXMAgent(QObject* parent = 0);
    // Copy
    PXMAgent(const PXMAgent& agent) = delete;
    // Copy Assignment
    PXMAgent& operator=(const PXMAgent& agent) = delete;
    // Move
    PXMAgent(PXMAgent&& agent) noexcept = delete;
    // Move Assignment
    PXMAgent& operator=(PXMAgent&& agent) noexcept = delete;
    // Destructor
    ~PXMAgent();

    static QPalette defaultPalette;
    static void changeToDark();

public slots:
    void doneChkUpdt(const QString &);
    int init();
private slots:
    int postInit();
signals:
    void alreadyRunning();
};

#endif  // PXMAGENT_H
