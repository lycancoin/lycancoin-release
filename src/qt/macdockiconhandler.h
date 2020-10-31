#ifndef MACDOCKICONHANDLER_H
#define MACDOCKICONHANDLER_H

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QIcon;
class QWidget;
QT_END_NAMESPACE

class objc_object;

/** Macintosh-specific dock icon handler.
 */
class MacDockIconHandler : public QObject
{
    Q_OBJECT

public:
    ~MacDockIconHandler();

    QMenu *dockMenu();
    void setIcon(const QIcon &icon);

    static MacDockIconHandler *instance();

    void handleDockIconClickEvent();

signals:
    void dockIconClicked();

private:
    MacDockIconHandler();

    objc_object *m_dockIconClickEventHandler;
    QWidget *m_dummyWidget;
    QMenu *m_dockMenu;
};

#endif // MACDOCKICONCLICKHANDLER_H
