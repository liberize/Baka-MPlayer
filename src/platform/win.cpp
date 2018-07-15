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
    QMap<QPair<HWND, UINT>, std::function<void(MSG *)> > handlers;

public:
    WinNativeEventFilter()
    {
        QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
    }

    ~WinNativeEventFilter()
    {
        QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);
    }

    void installHandler(HWND hwnd, UINT message, std::function<void(MSG *)> handler)
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
                it.value()(msg);
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

void SetAlwaysOnTop(QMainWindow *main, bool ontop)
{
    SetWindowPos((HWND)main->winId(),
                 ontop ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0,
                 SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

void SetAspectRatio(QMainWindow *main, int w, int h)
{
    if (!eventFilter) {
        eventFilter = new WinNativeEventFilter;
    }
    eventFilter->installHandler(main->winId(), WM_RESIZING, [] (MSG *msg) {
        int edge = int(msg->wParam);
        RECT &rect = *reinterpret_cast<LPRECT>(msg->lParam);
        switch (edge) {
        case WMSZ_BOTTOM:
        case WMSZ_TOP:
            // TODO:
            break;
        case WMSZ_BOTTOMLEFT:
            break;
        case WMSZ_BOTTOMRIGHT:
            break;
        case WMSZ_LEFT:
        case WMSZ_RIGHT:
            break;
        case WMSZ_TOPLEFT:
            break;
        case WMSZ_TOPRIGHT:
            break;
        }
    });
}

QString SettingsLocation()
{
    // saves to $(application directory)\${SETTINGS_FILE}.ini
    return QString("%0\\%1.ini").arg(QApplication::applicationDirPath(), SETTINGS_FILE);
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
    QProcess::startDetached("explorer.exe", QStringList{"/select,", path+file});
}

QString MonospaceFont()
{
    return "Lucida Console";
}

}
