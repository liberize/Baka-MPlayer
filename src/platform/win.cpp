#include "util.h"

#include <QApplication>
#include <QRegExp>
#include <QProcess>
#include <QDir>

#include <windows.h>

#include "settings.h"


namespace Util {

class WinNativeEventFilter : public QAbstractNativeEventFilter {
private:
    QMap<QPair<HWND, UINT>, std::function<bool(MSG *)> > handlers;

public:
    WinNativeEventFilter()
    {
        QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
    }

    ~WinNativeEventFilter()
    {
        QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);
    }

    void installHandler(HWND hwnd, UINT message, std::function<bool(MSG *)> handler)
    {
        QPair<HWND, UINT> key(hwnd, message);
        handlers[key] = handler;
    }

    void removeHandler(HWND hwnd, UINT message)
    {
        QPair<HWND, UINT> key(hwnd, message);
        handlers.remove(key);
    }

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *)
    {
        if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
            MSG *msg = (MSG *)message;
            QPair<HWND, UINT> key(msg->hwnd, msg->message);
            auto it = handlers.find(key);
            if (it != handlers.end())
                return it.value()(msg);
        }
        return false;
    }
};

static WinNativeEventFilter *eventFilter = nullptr;

QString VersionFileUrl()
{
    return "http://bakamplayer.u8sand.net/version_windows";
}

bool DimLightsSupported()
{
    return true;
}

void InitWindow(QMainWindow *main)
{
}

void SetAlwaysOnTop(QMainWindow *main, bool ontop)
{
    SetWindowPos((HWND)main->winId(),
                 ontop ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0,
                 SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

void SetAspectRatio(QMainWindow *main, int o_dwidth, int o_dheight)
{
    if (!eventFilter) {
        eventFilter = new WinNativeEventFilter;
    }
    eventFilter->installHandler(main->winId(), WM_SIZING, [] (MSG *msg) -> bool {
        RECT *rc = (RECT*)msg->lParam;
        // get client area of the windows if it had the rect rc
        // (subtracting the window borders)
        RECT b = { 0, 0, 0, 0 };
        AdjustWindowRect(&b, GetWindowLongPtrW(msg->hwnd, GWL_STYLE), 0);
        rc->left -= b.left;
        rc->top -= b.top;
        rc->right -= b.right;
        rc->bottom -= b.bottom;

        int c_w = rc->right - rc->left, c_h = rc->bottom - rc->top;
        float aspect = o_dwidth / (float) qMax(o_dheight, 1);
        int d_w = c_h * aspect - c_w;
        int d_h = c_w / aspect - c_h;
        int d_corners[4] = { d_w, d_h, -d_w, -d_h };
        int corners[4] = { rc->left, rc->top, rc->right, rc->bottom };
        int corner = -1;
        switch (msg->wParam) {
        case WMSZ_LEFT:         corner = 3; break;
        case WMSZ_TOP:          corner = 2; break;
        case WMSZ_RIGHT:        corner = 3; break;
        case WMSZ_BOTTOM:       corner = 2; break;
        case WMSZ_TOPLEFT:      corner = 1; break;
        case WMSZ_TOPRIGHT:     corner = 1; break;
        case WMSZ_BOTTOMLEFT:   corner = 3; break;
        case WMSZ_BOTTOMRIGHT:  corner = 3; break;
        }
        if (corner >= 0)
            corners[corner] -= d_corners[corner];
        *rc = (RECT) { corners[0], corners[1], corners[2], corners[3] };
        return true;
    });
}

bool IsValidFile(QString path)
{
    QRegExp rx("^(\\.{1,2}|[a-z]:|\\\\\\\\)", Qt::CaseInsensitive); // relative path, network location, drive
    return (rx.indexIn(path) != -1);
}

bool IsValidLocation(QString loc)
{
    QRegExp rx("^([a-z]{2,}://|\\.{1,2}|[a-z]:|\\\\\\\\)", Qt::CaseInsensitive); // url, relative path, network location, drive
    return (rx.indexIn(loc) != -1);
}

void ShowInFolder(QString path, QString file)
{
    QString args = file.isEmpty() ? path : "/select," + path + file;
    QProcess::startDetached("explorer.exe " + args);
}

QString MonospaceFont()
{
    return "Lucida Console";
}

}
