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
#include "mpvtypes.h"

namespace Ui {
class MainWindow;
}

class BakaEngine;
class MpvHandler;
class Plugin;
class MediaProvider;
class PlaylistWidget;

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
    PlaylistWidget *getPlaylistWidget();
    QString getRepeatType();
    void setRepeatType(QString r);

    Ui::MainWindow *ui;

public slots:
    void load(QString f = QString());
    void mapShortcuts();
    void registerPlugin(Plugin *plugin);
    QString getInput(QString title, QString prompt);

protected:
    void dragEnterEvent(QDragEnterEvent *event);    // drag file into
    void dropEvent(QDropEvent *event);              // drop file into
    void mousePressEvent(QMouseEvent *event);       // pressed mouse down
    void mouseReleaseEvent(QMouseEvent *event);     // released mouse up
    void mouseMoveEvent(QMouseEvent *event);        // moved mouse on the form
    void leaveEvent(QEvent *event);                 // mouse left the form
    void mouseDoubleClickEvent(QMouseEvent *event); // double clicked the form
    bool eventFilter(QObject *obj, QEvent *ev);  // event filter (get mouse move events from mpvFrame)
    void wheelEvent(QWheelEvent *event);            // the mouse wheel is used
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *event);

public slots:
    void setIndexLabels(bool enable);
    void enablePlaybackControls(bool enable);          // macro to enable/disable playback controls
    void enableTrackOperations(bool enable);
    void enableAudioFunctions(bool enable);
    void enableVideoFunctions(bool enable);
    void toggleSidebar(int index = -1);                          // toggles playlist visibility
    bool isSidebarVisible(int index = -1);                       // is the playlist visible?
    void showStartupPage(bool visible);
    void closeFile();

    void fullScreen(bool fullScreen, bool doubleClick = false);           // makes window fullscreen
    void showControls(bool visible, bool anim = true);
    void showSidebar(bool visible, bool anim = true, int index = -1);     // sets the playlist visibility
    void updateRecentFiles();                       // populate recentFiles menu
    void setPlayButtonIcon(bool play);
    void enableNextButton(bool enable);
    void enablePreviousButton(bool enable);
    void setRemainingLabels(double time);

private slots:
    void hideCursorAndControls();
    void showCursorAndControls(QMouseEvent *event);

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

private:
    BakaEngine *baka;
    MpvHandler *mpv;

#if defined(Q_OS_WIN)
    QWinThumbnailToolBar *thumbnailToolBar;
    QWinThumbnailToolButton *prevToolButton;
    QWinThumbnailToolButton *playPauseToolButton;
    QWinThumbnailToolButton *nextToolButton;
#endif
    bool fileChanged = false;
    bool menuVisible = true;
    bool firstItem = false;
    bool init = false;
    bool playlistState = false;
    QTimer *autoHideControls = nullptr;
    QTimer *updateBufferRange = nullptr;

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
    QMap<QString, QAction*> subtitlePluginActions;
    bool isContextMenuVisible = false;
};

#endif // MAINWINDOW_H
