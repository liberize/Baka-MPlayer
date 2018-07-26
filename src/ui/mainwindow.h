#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QTimer>
#include <QTranslator>
#include <QHash>
#include <QAction>
#include <QRect>
#include <QPropertyAnimation>

#if defined(Q_OS_WIN)
#include <QWinThumbnailToolBar>
#include <QWinThumbnailToolButton>
#endif

#include "recent.h"

namespace Ui {
class MainWindow;
}

class BakaEngine;
class MpvHandler;

class MainWindow : public QMainWindow {
friend class BakaEngine;
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QString getLang()          { return lang; }
    QString getOnTop()         { return onTop; }
    int getMaxRecent()         { return maxRecent; }
    bool getShowNotification() { return showNotification; }
    bool getRemaining()        { return remaining; }
    bool getScreenshotDialog() { return screenshotDialog; }
    bool getGestures()         { return gestures; }
    bool getResume()           { return resume; }

    QPointF getControlsCenterPos() { return controlsCenterPos; }
    void setControlsCenterPos(const QPointF &pos) { controlsCenterPos = pos; updateControlsPos(); }
    void updateControlsPos();
    int getSidebarWidth() { return sidebarWidth; }
    void setSidebarWidth(int width) { sidebarWidth = width; updateSidebarWidth(); }
    void updateSidebarWidth();
    QIcon getTrayIcon();

    Ui::MainWindow  *ui;
    QImage albumArt;

public slots:
    void Load(QString f = QString());
    void MapShortcuts();

protected:
    void dragEnterEvent(QDragEnterEvent *event);    // drag file into
    void dropEvent(QDropEvent *event);              // drop file into
    void mousePressEvent(QMouseEvent *event);       // pressed mouse down
    void mouseReleaseEvent(QMouseEvent *event);     // released mouse up
    void mouseMoveEvent(QMouseEvent *event);        // moved mouse on the form
    void leaveEvent(QEvent *event);                 // mouse left the form
    void mouseDoubleClickEvent(QMouseEvent *event); // double clicked the form
    bool eventFilter(QObject *obj, QEvent *event);  // event filter (get mouse move events from mpvFrame)
    void wheelEvent(QWheelEvent *event);            // the mouse wheel is used
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    void SetIndexLabels(bool enable);
    void EnablePlaybackControls(bool enable);          // macro to enable/disable playback controls
    void EnableTrackOperations(bool enable);
    void EnableAudioFunctions(bool enable);
    void EnableVideoFunctions(bool enable);
    void ToggleSidebar(int index = -1);                          // toggles playlist visibility
    bool isSidebarVisible(int index = -1);                       // is the playlist visible?
    void ShowStartupPage(bool visible);
    void CloseFile();

    void FullScreen(bool fullScreen, bool doubleClick = false);           // makes window fullscreen
    void ShowControls(bool visible, bool anim = true);
    void ShowSidebar(bool visible, bool anim = true, int index = -1);     // sets the playlist visibility
    void UpdateRecentFiles();                       // populate recentFiles menu
    void SetPlayButtonIcon(bool play);
    void EnableNextButton(bool enable);
    void EnablePreviousButton(bool enable);
    void SetRemainingLabels(int time);

private:
    BakaEngine      *baka;
    MpvHandler      *mpv;

#if defined(Q_OS_WIN)
    QWinThumbnailToolBar    *thumbnail_toolbar;
    QWinThumbnailToolButton *prev_toolbutton,
                            *playpause_toolbutton,
                            *next_toolbutton;
#endif
    bool pathChanged = false;
    bool menuVisible = true;
    bool firstItem = false;
    bool init = false;
    bool playlistState = false;
    QTimer *autohide = nullptr;

    // variables
    QList<Recent> recent;
    Recent *current = nullptr;
    QString lang;
    QString onTop;
    int maxRecent = false;
    bool showNotification = false;
    bool remaining = false;
    bool screenshotDialog = false;
    bool gestures = false;
    bool resume = true;
    QHash<QString, QAction*> commandActionMap;

    QPointF controlsCenterPos = QPointF(0.5, 0.2);
    QPoint controlsMoveStartPos = QPoint(-1, -1);
    int sidebarWidth = 200;
    int sidebarResizeStartX = -1;
#ifdef Q_OS_DARWIN
    int delayedFullScreen = -1;      // 0: normal, 1: fullscreen
#endif

public slots:
    void setLang(QString s)          { emit langChanged(lang = s); }
    void setOnTop(QString s)         { emit onTopChanged(onTop = s); }
    void setMaxRecent(int i)         { emit maxRecentChanged(maxRecent = i); }
    void setShowNotification(bool b) { emit showNotificationChanged(showNotification = b); }
    void setRemaining(bool b)        { emit remainingChanged(remaining = b); }
    void setScreenshotDialog(bool b) { emit screenshotDialogChanged(screenshotDialog = b); }
    void setGestures(bool b)         { emit gesturesChanged(gestures = b); }
    void setResume(bool b)           { emit resumeChanged(resume = b); }

signals:
    void langChanged(QString);
    void onTopChanged(QString);
    void maxRecentChanged(int);
    void showNotificationChanged(bool);
    void remainingChanged(bool);
    void screenshotDialogChanged(bool);
    void gesturesChanged(bool);
    void resumeChanged(bool);
};

#endif // MAINWINDOW_H
