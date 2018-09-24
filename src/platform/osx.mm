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
#include <QProcess>

#import <AppKit/Appkit.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

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

void InitWindow(QMainWindow *main)
{
    [NSWindow setAllowsAutomaticWindowTabbing:NO];
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];
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

void EnableScreenSaver(bool enable)
{
    static IOPMAssertionID assertionID = kIOPMNullAssertionID;

    if (enable) {
        if (assertionID == kIOPMNullAssertionID)
            return;
        IOReturn ret = IOPMAssertionRelease(assertionID);
        if (ret == kIOReturnSuccess)
            assertionID = kIOPMNullAssertionID;
    } else {
        if (assertionID != kIOPMNullAssertionID)
            return;
        IOReturn ret = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                kIOPMAssertionLevelOn, CFSTR("video playing"), &assertionID);
        Q_UNUSED(ret);
    }
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

void ShowInFolder(QString path, QString file)
{
    if (file.isEmpty())
        QDesktopServices::openUrl(QString("file:///%0").arg(path));
    else {
        QStringList scriptArgs;
        scriptArgs << QLatin1String("-e")
                   << QString::fromLatin1("tell application \"Finder\"\n"
                                          "  reveal POSIX file \"%1\"\n"
                                          "  activate\n"
                                          "end tell")
                                         .arg(Path(path, file));
        QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    }
}

QString MonospaceFont()
{
    return "Monospace";
}

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

}
