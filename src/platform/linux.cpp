#include "util.h"
#include "settings.h"

#include <QRegExp>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>

#include <QX11Info>
#include <X11/Xlib.h>

namespace Util {

QString VersionFileUrl()
{
    return "http://bakamplayer.u8sand.net/version_linux";
}

QString DownloadFileUrl()
{
    return "";
}

bool DimLightsSupported()
{
    QString tmp = "_NET_WM_CM_S"+QString::number(QX11Info::appScreen());
    Atom a = XInternAtom(QX11Info::display(), tmp.toUtf8().constData(), false);
    if (a && XGetSelectionOwner(QX11Info::display(), a)) // hack for QX11Info::isCompositingManagerRunning()
        return true;
    return false;
}

void InitWindow(QMainWindow *main)
{
}

void SetAlwaysOnTop(QMainWindow *main, bool ontop)
{
    // TODO: support wayland
    Display *display = QX11Info::display();
    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.display = display;
    event.xclient.window  = main->winId();
    event.xclient.message_type = XInternAtom (display, "_NET_WM_STATE", False);
    event.xclient.format = 32;

    event.xclient.data.l[0] = ontop;
    event.xclient.data.l[1] = XInternAtom (display, "_NET_WM_STATE_ABOVE", False);
    event.xclient.data.l[2] = 0; //unused.
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;

    XSendEvent(display, DefaultRootWindow(display), False,
                           SubstructureRedirectMask|SubstructureNotifyMask, &event);
}

void SetAspectRatio(QMainWindow *main, int w, int h)
{
    // TODO: support wayland
    Display *display = QX11Info::display();
    XSizeHints *hint = XAllocSizeHints();
    if (hint == NULL)
        return;

    hint->flags = PAspect;
    hint->min_aspect.x = w;
    hint->max_aspect.x = w;
    hint->min_aspect.y = h;
    hint->max_aspect.y = h;

    XSetWMNormalHints(display, main->winId(), hint);
    XFree(hint);
}

bool IsValidFile(QString path)
{
    QRegExp rx("^\\.{1,2}|/", Qt::CaseInsensitive); // relative path, network location, drive
    return (rx.indexIn(path) != -1);
}

bool IsValidLocation(QString loc)
{
    QRegExp rx("^([a-z]{2,}://|\\.{1,2}|/)", Qt::CaseInsensitive); // url, relative path, drive
    return (rx.indexIn(loc) != -1);
}

void ShowInFolder(QString path, QString)
{
    QDesktopServices::openUrl(QString("file:///%0").arg(path));
}

QString MonospaceFont()
{
    return "Monospace";
}

}
