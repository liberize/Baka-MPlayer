#include "util.h"
#include "settings.h"

#include <QRegExp>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
// not needed as SetAlwaysOnTop was stubbed for now
//#include <QWindow>
#include <QMainWindow>

#import <AppKit/Appkit.h>

namespace Util {

QString VersionFileUrl()
{
    return "http://bakamplayer.u8sand.net/version_osx";
}

QString DownloadFileUrl()
{
    return "";
}

bool DimLightsSupported()
{
    // stubbed
    return true;
}

void SetAlwaysOnTop(QMainWindow *main, bool ontop)
{
  if (ontop){
    // doesn't work
    /*
    QWindow *window = QWindow::fromWinId(main->winId());
    window->setFlags(
          Qt::WindowStaysOnTopHint
      );
    */
  }
}

void SetAspectRatio(QMainWindow *main, int w, int h)
{
    NSView *view = (__bridge NSView *)(void *)(main->winId());
    view.window.contentAspectRatio = NSMakeSize(w, h);
}

QString SettingsLocation()
{
    // saves to  ~/.config/${SETTINGS_FILE}.ini
    QString s1  = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString s2 = SETTINGS_FILE;
    return QString("%0/%1.ini").arg(
            QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
            SETTINGS_FILE);
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

#ifdef ENABLE_MPV_COCOA_WIDGET

void SetWantsLayer(QWidget *widget, bool wants)
{
    NSView *view = (__bridge NSView *)(void *)(widget->winId());
    view.wantsLayer = wants ? YES : NO;
}

void SetLayerOpaque(QWidget *widget, bool opaque)
{
    NSView *view = (__bridge NSView *)(void *)(widget->winId());
    view.layer.opaque = opaque ? YES : NO;
}

void SetLayerOpacity(QWidget *widget, double opacity)
{
    NSView *view = (__bridge NSView *)(void *)(widget->winId());
    view.layer.opacity = opacity;
}

void SetLayerBackgroundColor(QWidget *widget, double r, double g, double b, double a)
{
    NSView *view = (__bridge NSView *)(void *)(widget->winId());
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGColorRef color = CGColorCreate(colorSpace, (CGFloat[]) {r, g, b, a});
    view.layer.backgroundColor = color;
}

void SetLayerCornerRadius(QWidget *widget, double r)
{
    NSView *view = (__bridge NSView *)(void *)(widget->winId());
    view.layer.cornerRadius = r;
}

void SetCanDrawSubviewsIntoLayer(QWidget *widget)
{
    NSView *view = (__bridge NSView *)(void *)(widget->winId());
    view.canDrawSubviewsIntoLayer = YES;
}

#endif
}
