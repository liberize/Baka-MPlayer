#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtMath>
#include <QLibraryInfo>
#include <QMimeData>
#include <QDesktopWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QFileDialog>
#include <QDebug>
#include <QErrorMessage>

#include "bakaengine.h"
#include "mpvhandler.h"
#include "gesturehandler.h"
#include "overlayhandler.h"
#include "util.h"
#include "widgets/dimdialog.h"
#include "inputdialog.h"
#include "screenshotdialog.h"
#include "requestmanager.h"
#include "request.h"
#include "pluginmanager.h"
#include "subtitleprovider.h"
#include "mediaprovider.h"


MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
#ifndef Q_OS_DARWIN
    ui->menuHelp->insertSeparator(ui->actionAboutQt);
#endif

    Util::initWindow(this);
    ui->sidebarWidget->hide();
    autoHideControls = new QTimer(this);
    updateBufferRange = new QTimer(this);
    showStartupPage(true);

    // initialize managers/handlers
    baka = new BakaEngine(this);
    mpv = baka->mpv;

#ifdef ENABLE_MPV_COCOA_WIDGET
    // if enable MpvCocoaWidget, all views above the widget should be layer-backed
    Util::setWantsLayer(ui->controlsWidget, true);
    Util::setLayerOpaque(ui->controlsWidget, true);
    Util::setCanDrawSubviewsIntoLayer(ui->controlsWidget);
    Util::setLayerCornerRadius(ui->controlsWidget, 5);
    Util::setWantsLayer(ui->sidebarWidget, true);
    Util::setLayerOpaque(ui->sidebarWidget, true);
    Util::setCanDrawSubviewsIntoLayer(ui->sidebarWidget);
#endif

#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
    // update streaming support disabled on unix platforms
    ui->actionUpdateStreamingSupport->setEnabled(false);
#endif
    addActions(ui->menubar->actions()); // makes menubar shortcuts work even when menubar is hidden

    // set icons for start page buttons
    ui->playlistSearchBox->setIcon(QIcon(":/img/search.svg"), QSize(16, 16));
    ui->openFileButton->setIcon(QIcon(":/img/default_open.svg"), QSize(16, 16), 12);
    ui->openUrlButton->setIcon(QIcon(":/img/link.svg"), QSize(16, 16), 12);
    ui->viewLibraryButton->setIcon(QIcon(":/img/library.svg"), QSize(16, 16), 12);

    // initialize playlist widget and library widget
    ui->playlistWidget->attachEngine(baka);
    ui->libraryWidget->attachEngine(baka);
    ui->libraryWidget->setPlaceholderText(tr("No media provider selected."));

    // fix tab and line edit widget intercepting MouseMove event
    auto fixCursor = [=] (QMouseEvent *event) {
        if (sidebarResizeStartX < 0)
            unsetCursor();
        if (mpv->getPlayState() > 0)
            showCursorAndControls(event);
    };
    connect(ui->playlistWidget, &CustomListView::mouseMoved, fixCursor);
    connect(ui->libraryWidget, &CustomListView::mouseMoved, fixCursor);
    connect(ui->playlistSearchBox, &CustomLineEdit::mouseMoved, fixCursor);
    connect(ui->librarySearchBox, &CustomLineEdit::mouseMoved, fixCursor);

    // disable auto hide when context menu is visible
    auto fixCursor2 = [=] (bool visible) {
        isContextMenuVisible = visible;
        showCursorAndControls(nullptr);
    };
    connect(ui->librarySearchBox, &MediaSearchBox::menuVisibilityChanging, fixCursor2);
    connect(ui->playlistWidget, &PlaylistWidget::menuVisibilityChanging, fixCursor2);

    // install event filters for mpv container, playback controls widget and central widget
    ui->mpvContainer->installEventFilter(this);
    ui->controlsWidget->installEventFilter(this);
    ui->centralwidget->installEventFilter(this);

    QList<QWidget *> widgetList = findChildren<QWidget *>();
    for (auto &widget : widgetList) {
        widget->setMouseTracking(true);
    }

    // command action mappings (action (right) performs command (left))
    commandActionMap = {
        {"mpv add chapter +1", ui->actionNextChapter},
        {"mpv add chapter -1", ui->actionPreviousChapter},
        {"mpv set sub-scale 1", ui->actionResetFontSize},
        {"mpv add sub-scale +0.1", ui->actionIncreaseFontSize},
        {"mpv add sub-scale -0.1", ui->actionDecreaseFontSize},
        {"mpv cycle sub-visibility", ui->actionShowSubtitles},
        {"mpv set time-pos 0", ui->actionRestart},
        {"mpv frame_step", ui->actionFrameStep},
        {"mpv frame_back_step", ui->actionFrameBackStep},
        {"deinterlace", ui->actionDeinterlace},
        {"interpolate", ui->actionMotionInterpolation},
        {"mute", ui->actionMute},
        {"screenshot subtitles", ui->actionScreenshotWithSubtitles},
        {"screenshot", ui->actionScreenshotWithoutSubtitles},
        {"screenshot show", ui->actionOpenScreenshotFolder},
        {"add_subtitles", ui->actionAddSubtitleFile},
        {"add_audio", ui->actionAddAudioFile},
        {"video_size", ui->actionFitScreen},
        {"video_size 50", ui->actionVideoSize50},
        {"video_size 75", ui->actionVideoSize75},
        {"video_size 100", ui->actionVideoSize100},
        {"video_size 150", ui->actionVideoSize150},
        {"video_size 200", ui->actionVideoSize200},
        {"fullscreen", ui->actionFullScreen},
        {"jump", ui->actionJumpToTime},
        {"media_info", ui->actionMediaInfo},
        {"new", ui->actionNewPlayer},
        {"open", ui->actionOpenFile},
        {"open_clipboard", ui->actionOpenPathFromClipboard},
        {"open_location", ui->actionOpenURL},
        {"close", ui->actionClose},
        {"playlist play +1", ui->actionPlayNextFile},
        {"playlist play -1", ui->actionPlayPreviousFile},
        {"playlist repeat off", ui->actionRepeatOff},
        {"playlist repeat playlist", ui->actionRepeatPlaylist},
        {"playlist repeat this", ui->actionRepeatThisFile},
        {"playlist shuffle", ui->actionShuffle},
        {"playlist toggle", ui->actionShowPlaylist},
        {"library toggle", ui->actionShowLibrary},
        {"dim", ui->actionDimLights},
        {"play_pause", ui->actionPlay},
        {"quit", ui->actionExit},
        {"show_in_folder", ui->actionShowInFolder},
        {"stop", ui->actionStop},
        {"volume +5", ui->actionIncreaseVolume},
        {"volume -5", ui->actionDecreaseVolume},
        {"audio_delay +0.5", ui->actionIncreaseAudioDelay},       // "mpv add audio-delay +0.5"
        {"audio_delay -0.5", ui->actionDecreaseAudioDelay},       // "mpv add audio-delay -0.5"
        {"audio_delay 0", ui->actionResetAudioDelay},             // "mpv set audio-delay 0"
        {"subtitle_delay +0.5", ui->actionIncreaseSubtitleDelay}, // "mpv add sub-delay +0.5"
        {"subtitle_delay -0.5", ui->actionDecreaseSubtitleDelay}, // "mpv add sub-delay -0.5"
        {"subtitle_delay 0", ui->actionResetSubtitleDelay},       // "mpv set sub-delay 0"
        {"subtitle_font", ui->actionSubtitleFont},
        {"mpv add sub-pos -1", ui->actionSubtitleMoveUp},
        {"mpv add sub-pos +1", ui->actionSubtitleMoveDown},
        {"mpv set sub-pos 100", ui->actionResetSubtitlePos},
        {"subtitle_style color", ui->actionSubtitleColor},
        {"subtitle_style back-color", ui->actionSubtitleBackColor},
        {"subtitle_style blur", ui->actionSubtitleBlur},
        {"subtitle_style shadow-offset", ui->actionSubtitleShadowOffset},
        {"subtitle_style shadow-color", ui->actionSubtitleShadowColor},
        {"speed +0.1", ui->actionIncreaseSpeed},
        {"speed -0.1", ui->actionDecreaseSpeed},
        {"speed 2.0", ui->actionDoubleSpeed},
        {"speed 0.5", ui->actionHalfSpeed},
        {"speed 1.0", ui->actionResetSpeed},
        {"preferences", ui->actionPreferences},
        {"online_help", ui->actionOnlineHelp},
        {"update", ui->actionCheckForUpdates},
        {"update youtube-dl", ui->actionUpdateStreamingSupport},
        {"about qt", ui->actionAboutQt},
        {"about", ui->actionAboutBakaMPlayer}
    };

    // map actions to commands
    for (auto action = commandActionMap.begin(); action != commandActionMap.end(); ++action) {
        const QString cmd = action.key();
        connect(*action, &QAction::triggered, [=] { baka->command(cmd); });
    }

    // setup signals & slots

    // mainwindow
    connect(this, &MainWindow::langChanged, [=] (QString lang) {
        if (lang == "auto") // fetch lang from locale
            lang = QLocale::system().name();

        if (lang != "en") {
            QTranslator *tmp;

            // load the system translations provided by Qt
            tmp = baka->qtTranslator;
            baka->qtTranslator = new QTranslator();
            baka->qtTranslator->load(QString("qt_%0").arg(lang), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
            qApp->installTranslator(baka->qtTranslator);
            if (tmp != nullptr)
                delete tmp;

            // load the application translations
            tmp = baka->translator;
            baka->translator = new QTranslator();
            baka->translator->load(QString("baka-mplayer_%0").arg(lang), Util::translationsPath());
            qApp->installTranslator(baka->translator);
            if (tmp != nullptr)
                delete tmp;
        } else {
            if (baka->qtTranslator != nullptr)
                qApp->removeTranslator(baka->qtTranslator);
            if (baka->translator != nullptr)
                qApp->removeTranslator(baka->translator);
        }

        // save strings we want to keep
        QString title = windowTitle(),
                duration = ui->durationLabel->text(),
                remaining = ui->remainingLabel->text(),
                index = ui->indexLabel->text();

        ui->retranslateUi(this);

        // reload strings we kept
        setWindowTitle(title);
        ui->durationLabel->setText(duration);
        ui->remainingLabel->setText(remaining);
        ui->indexLabel->setText(index);
    });

    connect(this, &MainWindow::onTopChanged, [=] (QString onTop) {
        if (onTop == "never")
            Util::setAlwaysOnTop(this, false);
        else if (onTop == "always")
            Util::setAlwaysOnTop(this, true);
        else if (onTop == "playing" && mpv->getPlayState() > 0)
            Util::setAlwaysOnTop(this, true);
    });

    connect(this, &MainWindow::remainingChanged, [=] {
        setRemainingLabels(mpv->getTime());
    });

    connect(baka->sysTrayIcon, &QSystemTrayIcon::activated, [=] (QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (showNotification) {
                if (mpv->getPlayState() == Mpv::Playing)
                    baka->sysTrayIcon->showMessage("Baka MPlayer", tr("Playing"), QSystemTrayIcon::NoIcon, 4000);
                else if (mpv->getPlayState() == Mpv::Paused)
                    baka->sysTrayIcon->showMessage("Baka MPlayer", tr("Paused"), QSystemTrayIcon::NoIcon, 4000);
            }
            baka->playPause();
        }
    });

    // cursor autohide
    connect(autoHideControls, &QTimer::timeout, this, &MainWindow::hideCursorAndControls);

    connect(updateBufferRange, &QTimer::timeout, [=] {
        double current = mpv->getTime();
        double cached = mpv->getCacheTime();
        if (cached) {
            QList<QPair<double, double>> ranges = {{current, current + cached}};
            ui->seekBar->setBufferedRanges(ranges);
        }
    });

    // dim dialog
    connect(baka->dimDialog, &DimDialog::visbilityChanged, [=] (bool dim) {
        if (dim)
            Util::setAlwaysOnTop(this, true);
        else if (onTop == "never" || (onTop == "playing" && mpv->getPlayState() < 0))
            Util::setAlwaysOnTop(this, false);
    });

    // mpv

    connect(ui->playlistWidget, &PlaylistWidget::playlistChanged, [=] (QStandardItemModel *model) {
        if (model->rowCount() > 1) {
            ui->actionShuffle->setEnabled(true);
            ui->actionStopAfterCurrent->setEnabled(true);
        } else {
            ui->actionShuffle->setEnabled(false);
            ui->actionStopAfterCurrent->setEnabled(false);
        }
        if (model->rowCount() > 0)
            ui->menuRepeat->setEnabled(true);
        else
            ui->menuRepeat->setEnabled(false);
    });

    connect(mpv, &MpvHandler::fileInfoChanged, [=] (const Mpv::FileInfo &fileInfo) {
        if (mpv->getPlayState() > 0) {
            if (fileInfo.mediaTitle == "")
                setWindowTitle("Baka MPlayer");
            else if (fileInfo.mediaTitle == "-")
                setWindowTitle("Baka MPlayer: stdin"); // todo: disable playlist?
            else
                setWindowTitle(fileInfo.mediaTitle);

            bool save = mpv->getFileLocalOptions().isEmpty(); // don't save to recent list if has local file options
            QString f = mpv->getFile(), file = mpv->getPath() + f;
            if (save && f != QString() && maxRecent > 0) {
                int i = recent.indexOf(file);
                if (i >= 0) {
                    double t = recent.at(i).time;
                    if (t > 0 && resume)
                        mpv->seek(t);
                    recent.removeAt(i);
                }
                if (recent.isEmpty() || recent.front() != file) {
                    updateRecentFiles(); // update after initialization and only if the current file is different from the first recent
                    while (recent.length() > maxRecent - 1)
                        recent.removeLast();
                    recent.push_front(Recent(file, fileInfo.mediaTitle));
                    current = &recent.front();
                }
            }

            // reset speed if length isn't known and we have a streaming video
            // todo: don't save this reset, put their speed back when a normal video comes on
            // todo: disable speed alteration during streaming media
            if (fileInfo.length == 0)
                if (mpv->getSpeed() != 1)
                    mpv->setSpeed(1);

            ui->seekBar->setTotalTime(fileInfo.length);

            if (ui->actionMediaInfo->isChecked())
                baka->mediaInfo(true);

            setRemainingLabels(fileInfo.length);
        }
    });

    connect(ui->actionSubtitleTrackNone, &QAction::triggered, [=] {
        if (mpv->getSid() != 0)
            mpv->setSid(0);
        else
            ui->actionSubtitleTrackNone->setChecked(true);
    });

    connect(ui->actionAudioTrackNone, &QAction::triggered, [=] {
        if (mpv->getAid() != 0)
            mpv->setAid(0);
        else
            ui->actionAudioTrackNone->setChecked(true);
    });

    connect(ui->actionVideoTrackNone, &QAction::triggered, [=] {
        if (mpv->getVid() != 0)
            mpv->setVid(0);
        else
            ui->actionVideoTrackNone->setChecked(true);
    });

    connect(mpv, &MpvHandler::trackListChanged, [=] (const QList<Mpv::Track> &trackList) {
        ui->menuSubtitleTrack->clear();
        ui->menuSubtitleTrack->addAction(ui->actionSubtitleTrackNone);
        ui->menuAudioTrack->clear();
        ui->menuAudioTrack->addAction(ui->actionAudioTrackNone);
        ui->menuVideoTrack->clear();
        ui->menuVideoTrack->addAction(ui->actionVideoTrackNone);

        if (mpv->getPlayState() > 0) {
            QAction *action;
            QString title;
            bool hasAudio = false, hasVideo = false;
            for (auto &track : trackList) {
                if (track.type == "sub") {
                    title = mpv->formatTrackInfo(track);
                    action = ui->menuSubtitleTrack->addAction(title);
                    connect(action, &QAction::triggered, [=] {
                        if (mpv->getSid() != track.id) {
                            mpv->setSid(track.id);
                            mpv->showText(QString("%0 %1").arg(tr("Sub"), title));
                        } else
                            action->setChecked(true);
                    });
                    if (mpv->getSid() == track.id) {
                        action->setCheckable(true);
                        action->setChecked(true);
                    }
                } else if (track.type == "audio") {
                    hasAudio = true;
                    title = mpv->formatTrackInfo(track);
                    action = ui->menuAudioTrack->addAction(title);
                    connect(action, &QAction::triggered, [=] {
                        if (mpv->getAid() != track.id) {
                            mpv->setAid(track.id);
                            mpv->showText(QString("%0 %1").arg(tr("Audio"), title));
                        } else
                            action->setChecked(true);
                    });
                    if (mpv->getAid() == track.id) {
                        action->setCheckable(true);
                        action->setChecked(true);
                    }
                } else if (track.type == "video") { // video track
                    if (!track.albumArt) // isn't album art
                        hasVideo = true;
                    title = mpv->formatTrackInfo(track);
                    action = ui->menuVideoTrack->addAction(title);
                    connect(action, &QAction::triggered, [=] {
                        if (mpv->getVid() != track.id) {
                            mpv->setVid(track.id);
                            mpv->showText(QString("%0 %1").arg(tr("Video"), title));
                        } else
                            action->setChecked(true);
                    });
                    if (mpv->getVid() == track.id) {
                        action->setCheckable(true);
                        action->setChecked(true);
                    }
                }
            }

            enableTrackOperations(true);
            enableAudioFunctions(hasAudio);
            enableVideoFunctions(hasVideo);

            if (hasAudio && !hasVideo && baka->sysTrayIcon->isVisible() && showNotification) {
                // todo: use {artist} - {title}
                baka->sysTrayIcon->showMessage("Baka MPlayer", mpv->getFileInfo().mediaTitle, QSystemTrayIcon::NoIcon, 4000);
            }
        } else {
            enableTrackOperations(false);
            enableAudioFunctions(false);
            enableVideoFunctions(false);
        }
    });

    connect(mpv, &MpvHandler::chaptersChanged, [=] (const QList<Mpv::Chapter> &chapters) {
        if (mpv->getPlayState() > 0) {
            QAction *action;
            QList<double> ticks;
            int n = 1, N = chapters.length();
            ui->menuChapters->clear();
            for (auto &ch : chapters) {
                action = ui->menuChapters->addAction(QString("%0: %1").arg(Util::formatNumberWithAmpersand(n, N), ch.title));
                if (n <= 9)
                    action->setShortcut(QKeySequence("Ctrl+" + QString::number(n)));
                connect(action, &QAction::triggered, [=] {
                    mpv->seek(ch.time);
                });
                ticks.push_back(ch.time);
                n++;
            }
            if (ui->menuChapters->actions().count() == 0) {
                ui->menuChapters->setEnabled(false);
                ui->actionNextChapter->setEnabled(false);
                ui->actionPreviousChapter->setEnabled(false);
            } else {
                ui->menuChapters->setEnabled(true);
                ui->actionNextChapter->setEnabled(true);
                ui->actionPreviousChapter->setEnabled(true);
            }

            ui->seekBar->setChapterTicks(ticks);
        }
    });

    connect(mpv, &MpvHandler::playStateChanged, [=] (Mpv::PlayState playState) {
        switch(playState) {
        case Mpv::Loaded:
            baka->mpv->showText("Loading...", 0);
            break;

        case Mpv::Started:
            if (!init) { // will only happen the first time a file is loaded.
                ui->actionPlay->setEnabled(true);
                ui->playButton->setEnabled(true);
#if defined(Q_OS_WIN)
                playPauseToolButton->setEnabled(true);
#endif
                ui->actionShowPlaylist->setEnabled(true);
                ui->actionShowLibrary->setEnabled(true);
                ui->menuSubtitleTrack->setEnabled(true);
                ui->menuAudioTrack->setEnabled(true);
                ui->menuVideoTrack->setEnabled(true);
                init = true;
            }
            enablePlaybackControls(true);
            mpv->play();
            baka->overlay->showStatusText(QString(), 0);
        case Mpv::Playing:
            setPlayButtonIcon(false);
            if (onTop == "playing")
                Util::setAlwaysOnTop(this, true);
            Util::enableScreenSaver(false);
            break;

        case Mpv::Stopped:
            current = nullptr;
            updateRecentFiles();
        case Mpv::Paused:
            setPlayButtonIcon(true);
            if (onTop == "playing")
                Util::setAlwaysOnTop(this, false);
            Util::enableScreenSaver(true);
            break;

        case Mpv::Idle:
            bool stop = true;
            if (init) {
                auto repeat = getRepeatType();
                bool stopAfterCurrent = ui->actionStopAfterCurrent->isChecked();
                int playingRow = ui->playlistWidget->playingRow();
                int rowCount = ui->playlistWidget->count();
                if (repeat == "this") {
                    if (playingRow != -1) {
                        ui->playlistWidget->playRow(0, true); // restart file
                        stop = false;
                    }
                } else if (stopAfterCurrent || playingRow >= rowCount - 1 || playingRow == -1) {
                    if (!stopAfterCurrent && repeat == "playlist" && rowCount > 0) {
                        ui->playlistWidget->playRow(0); // restart playlist
                        stop = false;
                    }
                } else {
                    ui->playlistWidget->playRow(1, true);
                    stop = false;
                }
            }
            if (stop) {
                setWindowTitle("Baka MPlayer");
                Util::setAspectRatio(this, 0, 0);
                QRect rect(0, 0, 640, 480);
                rect.moveCenter(geometry().center());
                setGeometry(rect);
                enablePlaybackControls(false);
                ui->seekBar->setTotalTime(0);
                ui->actionStopAfterCurrent->setChecked(false);
                showStartupPage(true);
            }
            break;
        }
    });

    connect(mpv, &MpvHandler::fileChanging, [=] (double t, double l) {
        if (current != nullptr)
            current->time = (t > 0.05 * l && t < 0.95 * l) ? t : 0;
        showStartupPage(false);
    });

    connect(mpv, &MpvHandler::fileChanged, [=] (QString) {
        fileChanged = true;
    });

    connect(mpv, &MpvHandler::videoParamsChanged, [=] (const Mpv::VideoParams &) {
        if (fileChanged) {
            fileChanged = false;
            baka->fitWindow(100, false);
        }
    });

    connect(mpv, &MpvHandler::timeChanged, [=] (double i) {
        const Mpv::FileInfo &fi = mpv->getFileInfo();
        // set the seekBar's location with NoSignal function so that it doesn't trigger a seek
        // the formula is a simple ratio seekBar's max * time/totalTime
        ui->seekBar->setValueNoSignal(ui->seekBar->maximum() * (i / fi.length));
        setRemainingLabels(i);

        // set next/previous chapter's enabled state
        if (fi.chapters.length() > 0) {
            ui->actionNextChapter->setEnabled(i < fi.chapters.last().time);
            ui->actionPreviousChapter->setEnabled(i > fi.chapters.first().time);
        }
    });

    connect(mpv, &MpvHandler::volumeChanged, ui->volumeSlider, &CustomSlider::setValueNoSignal);

    connect(mpv, &MpvHandler::speedChanged, [=] (double speed) {
        static double last = 1;
        if (last != speed) {
            if (init)
                mpv->showText(tr("Speed: %0x").arg(QString::number(speed)));
            if (speed <= 0.25)
                ui->actionDecreaseSpeed->setEnabled(false);
            else
                ui->actionDecreaseSpeed->setEnabled(true);
            last = speed;
        }
    });

    // subtitle track changed
    connect(mpv, &MpvHandler::sidChanged, [=] (int sid) {
        QList<QAction*> actions = ui->menuSubtitleTrack->actions();
        for (auto &action : actions) {
            if ((!sid && action == ui->actionSubtitleTrackNone) || (sid && action->text().startsWith(QString::number(sid)))) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    // audio track changed
    connect(mpv, &MpvHandler::aidChanged, [=] (int aid) {
        QList<QAction*> actions = ui->menuAudioTrack->actions();
        for (auto &action : actions) {
            if (mpv->getPlayState() > 0 && ((!aid && action == ui->actionAudioTrackNone) ||
                                            (aid && action->text().startsWith(QString::number(aid))))) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    // video track changed
    connect(mpv, &MpvHandler::vidChanged, [=] (int vid) {
        QList<QAction*> actions = ui->menuVideoTrack->actions();
        for (auto &action : actions) {
            if (mpv->getPlayState() > 0 && ((!vid && action == ui->actionVideoTrackNone) ||
                                            (vid && action->text().startsWith(QString::number(vid))))) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::subtitleVisibilityChanged, [=] (bool b) {
        if (ui->actionShowSubtitles->isEnabled())
            ui->actionShowSubtitles->setChecked(b);
        if (init)
            mpv->showText(b ? tr("Subtitles visible") : tr("Subtitles hidden"));
    });

    connect(mpv, &MpvHandler::muteChanged, [=] (bool b) {
        if (b)
            ui->muteButton->setIcon(QIcon(":/img/default_mute.svg"));
        else
            ui->muteButton->setIcon(QIcon(":/img/default_unmute.svg"));
        mpv->showText(b ? tr("Muted") : tr("Unmuted"));
    });

    connect(mpv, &MpvHandler::voChanged, [=] (QString vo) {
        ui->actionMotionInterpolation->setChecked(vo.contains("interpolation"));
    });

    connect(mpv, &MpvHandler::audioDeviceChanged, [=] (QString device) {
        QList<QAction*> actions = ui->menuAudioDevice->actions();
        for (auto &action : actions) {
            if (action->text().endsWith(device)) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::audioDeviceListChanged, [=] (const QList<Mpv::AudioDevice> &deviceList) {
        ui->menuAudioDevice->clear();
        for (const auto &device : deviceList) {
            QString title = QString("[%0] %1").arg(device.description, device.name);
            QAction *action = ui->menuAudioDevice->addAction(title);
            connect(action, &QAction::triggered, [=] {
                if (mpv->getAudioDevice() != device.name)
                    mpv->setAudioDevice(device.name);
                else
                    action->setChecked(true);
            });
        }
    });

    connect(mpv, &MpvHandler::subtitleEncodingChanged, [=] (QString encoding) {
        QString title = Util::getCharEncodingTitle(encoding);
        QList<QAction*> actions = ui->menuSubtitleEncoding->actions();
        for (auto &action : actions) {
            if (action->text() == title) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::subtitleEncodingListChanged, [=] (const QList<QPair<QString, QString> > &encodingList) {
        ui->menuSubtitleEncoding->clear();
        for (const auto &pair : encodingList) {
            QAction *action = ui->menuSubtitleEncoding->addAction(pair.second);
            connect(action, &QAction::triggered, [=] {
                if (mpv->getSubtitleEncoding() != pair.first)
                    mpv->setSubtitleEncoding(pair.first);
                else
                    action->setChecked(true);
            });
        }
    });

    // ui

    // playback: seekbar clicked
    connect(ui->seekBar, &SeekBar::valueChanged, [=] (int i) {
        mpv->seek(mpv->relative(((double)i / ui->seekBar->maximum()) * mpv->getFileInfo().length), true);
    });

    connect(ui->openFileButton, &QPushButton::clicked, baka, &BakaEngine::open);

    connect(ui->openUrlButton, &QPushButton::clicked, baka, &BakaEngine::openLocation);

    connect(ui->viewLibraryButton, &QPushButton::clicked, [=] {
        showSidebar(true, true, 1);
    });

    // playback: previous button clicked
    connect(ui->previousButton, &QPushButton::clicked, [=] {
        ui->playlistWidget->playRow(-1, true);
    });

    // playback: play/pause button clicked
    connect(ui->playButton, &QPushButton::clicked, baka, &BakaEngine::playPause);

    // playback: next button clicked
    connect(ui->nextButton, &QPushButton::clicked, [=] {
        ui->playlistWidget->playRow(1, true);
    });

    // playback: mute button clicked
    connect(ui->muteButton, &QPushButton::clicked, [=] {
        mpv->setMute(!mpv->getMute());
    });

    // playback: volume slider adjusted
    connect(ui->volumeSlider, &CustomSlider::valueChanged, [=] (int i) {
        mpv->setVolume(i, true);
    });

    connect(ui->sidebarButton, &QPushButton::clicked, this, &MainWindow::toggleSidebar);

    // playlist: search box text changed
    connect(ui->playlistSearchBox, &QLineEdit::textChanged, ui->playlistWidget, &PlaylistWidget::search);

    // playlist: playlist selection changed
    connect(ui->playlistWidget, &PlaylistWidget::currentRowChanged, this, [=] (int) {
        setIndexLabels(true);
    }, Qt::QueuedConnection);

    // playlist: repeat button clicked, switch between off/single/list
    connect(ui->repeatButton, &QPushButton::clicked, [=] {
        if (ui->actionRepeatOff->isChecked())
            ui->actionRepeatPlaylist->trigger();
        else if (ui->actionRepeatPlaylist->isChecked())
            ui->actionRepeatThisFile->trigger();
        else if (ui->actionRepeatThisFile->isChecked())
            ui->actionRepeatOff->trigger();
    });

    connect(ui->shuffleButton, &QPushButton::clicked, ui->actionShuffle, &QAction::trigger);

    // playlist: add button clicked, add item to playlist
    connect(ui->addButton, &QPushButton::clicked, [=] {
        QString file = QFileDialog::getOpenFileName(this, tr("Add File to Playlist"), mpv->getPath(),
                                                    QString("%0 (%1);;").arg(tr("Media Files"), Mpv::MEDIA_FILE_TYPES.join(" ")) +
                                                    QString("%0 (%1);;").arg(tr("Video Files"), Mpv::VIDEO_FILE_TYPES.join(" ")) +
                                                    QString("%0 (%1);;").arg(tr("Audio Files"), Mpv::AUDIO_FILE_TYPES.join(" ")) +
                                                    QString("%0 (*.*)").arg(tr("All Files")),
                                                    0, QFileDialog::DontUseSheet);
        if (!file.isEmpty())
            ui->playlistWidget->addItem(QFileInfo(file).fileName(), file, true);
    });

    // playlist: clear button clicked
    connect(ui->clearButton, &QPushButton::clicked, ui->playlistWidget, &PlaylistWidget::clearNotPlaying);

    // library: provider changed
    connect(ui->librarySearchBox, &MediaSearchBox::providerChanged, [=] (MediaProvider *provider) {
        ui->libraryWidget->clear();
        baka->pluginManager->clearWorkers(Worker::Low);
        if (provider) {
            QString word = ui->librarySearchBox->getWord();
            if (word.isEmpty()) {
                ui->libraryWidget->setPlaceholderText(tr("Fetching..."));
                provider->fetch(0);
            } else {
                ui->libraryWidget->setPlaceholderText(tr("Searching..."));
                provider->search(word);
            }
        } else
            ui->libraryWidget->setPlaceholderText(tr("No media provider selected."));
    });

    // library: search box submitted
    connect(ui->librarySearchBox, &CustomLineEdit::submitted, [=] (QString text) {
        ui->libraryWidget->clear();
        baka->pluginManager->clearWorkers(Worker::Low);
        MediaProvider *provider = ui->librarySearchBox->getCurrentProvider();
        if (provider) {
            if (text.isEmpty()) {
                ui->libraryWidget->setPlaceholderText(tr("Fetching..."));
                provider->fetch(0);
            } else {
                ui->libraryWidget->setPlaceholderText(tr("Searching..."));
                provider->search(text);
            }
        }
    });

    // playlist: item double clicked
    connect(ui->playlistWidget, &QListView::doubleClicked, [=] (const QModelIndex &index) {
        ui->playlistWidget->playIndex(index);
        showSidebar(false);
    });

    // library: item double clicked
    connect(ui->libraryWidget, &QListView::doubleClicked, [=] (const QModelIndex &index) {
        MediaEntry *entry = index.data(Qt::UserRole).value<MediaEntry*>();
        MediaProvider *provider = ui->librarySearchBox->getCurrentProvider();
        if (provider)
            provider->download(*entry, "", index);
        showSidebar(false);
    });

    // library: scroll to end, fetch more items
    connect(ui->libraryWidget, &LibraryWidget::scrollReachedEnd, [=] () {
        MediaProvider *provider = ui->librarySearchBox->getCurrentProvider();
        if (provider) {
            QString word = ui->librarySearchBox->getWord();
            if (word.isEmpty())
                provider->fetchNext();
        }
    });

    // add multimedia shortcuts
    ui->actionPlay->setShortcuts({ui->actionPlay->shortcut(), QKeySequence(Qt::Key_MediaPlay)});
    ui->actionStop->setShortcuts({ui->actionStop->shortcut(), QKeySequence(Qt::Key_MediaStop)});
    ui->actionPlayNextFile->setShortcuts({ui->actionPlayNextFile->shortcut(), QKeySequence(Qt::Key_MediaNext)});
    ui->actionPlayPreviousFile->setShortcuts({ui->actionPlayPreviousFile->shortcut(), QKeySequence(Qt::Key_MediaPrevious)});
}

MainWindow::~MainWindow()
{
    closeFile();
    baka->saveSettings();

    // Note: child objects _should_ not need to be deleted because
    // all children should get deleted when mainwindow is deleted
    // see: http://qt-project.org/doc/qt-4.8/objecttrees.html

    // but apparently they don't (https://github.com/u8sand/Baka-MPlayer/issues/47)
#if defined(Q_OS_WIN)
    delete prevToolButton;
    delete playPauseToolButton;
    delete nextToolButton;
    delete thumbnailToolBar;
#endif
    delete baka;
    delete ui;
}

void MainWindow::registerPlugin(Plugin *plugin)
{
    if (plugin->isSubtitleProvider()) {
        SubtitleProvider *provider = static_cast<SubtitleProvider*>(plugin);
        QString name = provider->getName();

        connect(provider, &Plugin::enableStateChanged, [=] (bool enable) {
            if (enable) {
                // add separator
                if (subtitlePluginActions.empty())
                    ui->menuSubtitles->addSeparator();

                // add download subtitle menu
                QAction *action = new QAction(tr("Download from \"%0\"...").arg(name), this);
                ui->menuSubtitles->addAction(action);
                subtitlePluginActions[name] = action;
                connect(action, &QAction::triggered, [=] {
                    QString word = mpv->getFileInfo().mediaTitle;
                    if (word.isEmpty()) {
                        QFileInfo info(mpv->getFile());
                        QString base = info.completeBaseName();
                        if (mpv->getPath().isEmpty() || info.suffix().isEmpty() ||
                                base.length() <= 3 || QRegExp("\\d*").exactMatch(base)) {
                            word = InputDialog::getInput(tr("Input a word to search"), tr("Search Subtitles"), [=] (QString text) {
                                return text.length() >= 2;
                            }, this);
                            if (word.isEmpty())
                                return;
                        } else
                            word = base;
                    }
                    provider->search(word);
                });
                action->setEnabled(mpv->hasVideo());
            } else {
                // remove download subtitle menu
                if (QAction *action = subtitlePluginActions.value(name, nullptr)) {
                    ui->menuSubtitles->removeAction(action);
                    subtitlePluginActions.remove(name);
                    action->deleteLater();
                }
                // remove subtitle list menu
                if (QAction *action = subtitlePluginActions.value(name + " Result", nullptr)) {
                    ui->menuSubtitles->removeAction(action);
                    subtitlePluginActions.remove(name + " Result");
                    action->menu()->deleteLater();
                }
                // remove separators
                auto actions = ui->menuSubtitles->actions();
                if (subtitlePluginActions.empty() && actions.back()->isSeparator())
                    ui->menuSubtitles->removeAction(actions.back());
            }
        });

        connect(provider, &SubtitleProvider::searchFinished, [=] (const QList<SubtitleEntry> &result) {
            QMenu *menu = nullptr;
            // clear or create subtitle list menu
            QAction *act = subtitlePluginActions.value(name + " Result", nullptr);
            if (act) {
                menu = act->menu();
                menu->clear();
            } else {
                menu = new QMenu(tr("Subtitles from \"%0\"").arg(name), this);
                ui->menuSubtitles->insertMenu(ui->actionAddSubtitleFile, menu);
                subtitlePluginActions[name + " Result"] = menu->menuAction();
            }
            // populate subtitle list submenu
            for (auto &entry : result) {
                QAction *act = menu->addAction(entry.name);
                connect(act, &QAction::triggered, [=] {
                    QString localFile = Util::toLocalFile(entry.url);
                    if (!localFile.isEmpty())
                        mpv->addSubtitleTrack(localFile);
                    else
                        provider->download(entry);
                });
            }
            menu->setEnabled(mpv->hasVideo());
        });

        connect(provider, &SubtitleProvider::downloadFinished, [=] (const SubtitleEntry &entry) {
            QString localFile = Util::toLocalFile(entry.url);
            if (!localFile.isEmpty()) {
                // if url is local file, add it directly
                mpv->addSubtitleTrack(localFile);
            } else if (Util::isValidUrl(entry.url)) {
                // if url is valid web url, download it first
                Request *req = baka->requestManager->newRequest(entry.url);
                connect(req, &Request::error, [=] (QString msg) {
                    mpv->showText(tr("Download failed with error: %0").arg(msg));
                    req->deleteLater();
                });
                connect(req, &Request::saved, [=] (QString filePath) {
                    mpv->addSubtitleTrack(filePath);
                    req->deleteLater();
                });
                mpv->showText(tr("Downloading %0...").arg(entry.name));
                req->fetch(true);
            } else
                mpv->showText(tr("Invalid subtitle path: %0").arg(entry.url));
        });

        connect(provider, &SubtitleProvider::error, [=] (QString msg) {
            QErrorMessage errMsg(this);
            errMsg.showMessage(msg);
        });

    } else if (plugin->isMediaProvider()) {
        MediaProvider *provider = static_cast<MediaProvider*>(plugin);
        QString name = provider->getName();

        connect(provider, &Plugin::enableStateChanged, [=] (bool enable) {
            if (enable)
                ui->librarySearchBox->addProvider(provider);
            else
                ui->librarySearchBox->removeProvider(provider);
        });

        auto populateMediaList = [=] (const QList<MediaEntry> &result) {
            // show place holder if result is empty
            if (result.isEmpty()) {
                ui->libraryWidget->setPlaceholderText(tr("No result found."));
                ui->libraryWidget->update();
                return;
            }
            // populate media list, if cover is null, try to download it
            auto provider = ui->librarySearchBox->getCurrentProvider();
            for (auto &entry : result) {
                MediaEntry *e = new MediaEntry(entry);
                QPersistentModelIndex index = ui->libraryWidget->appendEntry(e);
                if (entry.cover.isNull())
                    provider->download(entry, "cover", index);
            }
        };
        connect(provider, &MediaProvider::fetchFinished, populateMediaList);
        connect(provider, &MediaProvider::searchFinished, populateMediaList);

        connect(provider, &MediaProvider::downloadFinished, [=] (const MediaEntry &entry, QString what, const QPersistentModelIndex &index) {
            if (what == "") {
                // media download finished
                // in most cases, plugin simply translates url instead of downloading it actually
                mpv->playFile(entry.url, entry.name, entry.options);
            } else if (what == "cover") {
                // cover download finished
                // if entry not exist any more, do nothing
                MediaEntry *item = index.data(Qt::UserRole).value<MediaEntry*>();
                if (!item)
                    return;
                QString localFile = Util::toLocalFile(entry.coverUrl);
                if (!localFile.isEmpty()) {
                    // if url is local file, load it directly
                    item->cover.load(localFile);
                    ui->libraryWidget->update(index);
                } else if (Util::isValidUrl(entry.coverUrl)) {
                    // if url is valid web url, download it first
                    Request *req = baka->requestManager->newRequest(entry.coverUrl);
                    connect(req, &Request::error, [=] (QString) {
                        req->deleteLater();
                    });
                    connect(req, &Request::saved, [=] (QString filePath) {
                        item->cover.load(filePath);
                        ui->libraryWidget->update(index);
                        req->deleteLater();
                    });
                    req->fetch(true);
                }
            }
        });

        connect(provider, &MediaProvider::error, [=] (QString msg) {
            if (ui->libraryWidget->count() == 0) {
                ui->libraryWidget->setPlaceholderText(msg);
                ui->libraryWidget->update();
            }
        });
    }
}

void MainWindow::load(QString file)
{
    // load the settings here--the constructor has already been called
    // this solves some issues with setting things before the constructor has ended
    menuVisible = ui->menubar->isVisible(); // does the OS use a menubar? (appmenu doesn't)
#if defined(Q_OS_WIN)
    // add windows 7+ thubnail toolbar buttons
    thumbnailToolBar = new QWinThumbnailToolBar(this);
    thumbnailToolBar->setWindow(this->windowHandle());

    prevToolButton = new QWinThumbnailToolButton(thumbnailToolBar);
    prevToolButton->setEnabled(false);
    prevToolButton->setToolTip(tr("Previous"));
    prevToolButton->setIcon(QIcon(":/img/tool-previous.ico"));
    connect(prevToolButton, &QWinThumbnailToolButton::clicked, [=] {
        ui->playlistWidget->playRow(-1, true);
    });

    playPauseToolButton = new QWinThumbnailToolButton(thumbnailToolBar);
    playPauseToolButton->setEnabled(false);
    playPauseToolButton->setToolTip(tr("Play"));
    playPauseToolButton->setIcon(QIcon(":/img/tool-play.ico"));
    connect(playPauseToolButton, &QWinThumbnailToolButton::clicked, baka, &BakaEngine::playPause);

    nextToolButton = new QWinThumbnailToolButton(thumbnailToolBar);
    nextToolButton->setEnabled(false);
    nextToolButton->setToolTip(tr("Next"));
    nextToolButton->setIcon(QIcon(":/img/tool-next.ico"));
    connect(nextToolButton, &QWinThumbnailToolButton::clicked, [=] {
        ui->playlistWidget->playRow(1, true);
    });

    thumbnailToolBar->addButton(prevToolButton);
    thumbnailToolBar->addButton(playPauseToolButton);
    thumbnailToolBar->addButton(nextToolButton);
#endif
    baka->loadSettings();
    baka->loadPlugins();
    mpv->initialize();
    mpv->loadAudioDevices();
    mpv->loadSubtitleEncodings();
    mpv->playFile(file);
}

void MainWindow::updateControlsPos()
{
    int parentWidth = ui->centralwidget->width();
    int parentHeight = ui->centralwidget->height();
    int width = ui->controlsWidget->width();
    int height = ui->controlsWidget->height();
    int controlsPosX = int(parentWidth * controlsCenterPos.x() - width * 0.5);
    controlsPosX = qMax(qMin(controlsPosX, parentWidth - width), 0);
    int controlsPosY = int(parentHeight * controlsCenterPos.y() - height * 0.5);
    controlsPosY = qMax(parentHeight - height - qMax(controlsPosY, 0), 0);
    ui->controlsWidget->move(controlsPosX, controlsPosY);
}

void MainWindow::updateSidebarWidth()
{
    int parentWidth = ui->centralwidget->width();
    int parentHeight = ui->centralwidget->height();
    int newWidth = qMax(qMin(sidebarWidth, parentWidth / 2), 100);
    ui->sidebarWidget->setGeometry(parentWidth - newWidth, 0, newWidth, parentHeight);
}

QIcon MainWindow::getTrayIcon()
{
#ifdef Q_OS_DARWIN
    QIcon icon(":/img/logo_mask.svg");
    icon.setIsMask(true);
    return icon;
#else
    return windowIcon();
#endif
}

PlaylistWidget *MainWindow::getPlaylistWidget()
{
    return ui->playlistWidget;
}

QString MainWindow::getRepeatType()
{
    if (ui->actionRepeatPlaylist->isChecked())
        return "playlist";
    if (ui->actionRepeatThisFile->isChecked())
        return "this";
    return "off";
}

void MainWindow::setRepeatType(QString r)
{
    if (r == "playlist")
        ui->actionRepeatPlaylist->trigger();
    else if (r == "this")
        ui->actionRepeatThisFile->trigger();
    else if (r == "off")
        ui->actionRepeatOff->trigger();
}

QString MainWindow::getInput(QString title, QString prompt)
{
    return InputDialog::getInput(prompt, title, [=] (QString input) {
        return !input.isEmpty();
    }, this);
}

void MainWindow::mapShortcuts()
{
    auto tmp = commandActionMap;
    // map shortcuts to actions
    for (auto input_iter = baka->input.begin(); input_iter != baka->input.end(); ++input_iter) {
        auto commandAction = tmp.find(input_iter->first);
        if (commandAction != tmp.end()) {
            (*commandAction)->setShortcut(QKeySequence(input_iter.key()));
            tmp.erase(commandAction);
        }
    }
    // clear the rest
    for (auto iter = tmp.begin(); iter != tmp.end(); ++iter)
        (*iter)->setShortcut(QKeySequence());
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) // url / text
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) { // urls
        for (QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                QString file = url.toLocalFile();
                QRegExp re(Mpv::SUBTITLE_FILE_TYPES.join('|').replace('.', "\\.").replace('*', ".*"));
                if (re.exactMatch(file))
                    mpv->addSubtitleTrack(file);
                else
                    mpv->playFile(file);
            } else
                mpv->playFile(url.url());
        }
    } else if (mimeData->hasText()) // text
        mpv->playFile(mimeData->text());
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!isFullScreen()) // not fullscreen
            baka->gesture->begin(GestureHandler::MOVE, event->globalPos(), pos());
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        baka->gesture->end();
    }
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::leaveEvent(QEvent *event)
{
    showControls(false);
    QMainWindow::leaveEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == ui->controlsWidget) {
        if (ev->type() == QEvent::MouseButtonPress) {
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                if (ui->remainingLabel->rect().contains(ui->remainingLabel->mapFrom(ui->controlsWidget, event->pos()))) {
                    // left click on remaining label, toggle display
                    setRemaining(!remaining);
                    return true;
                } else {
                    // sidebar resizing has higher priority than controls dragging
                    // only start dragging if cursor not around sidebar left border
                    QRect coldArea(ui->sidebarWidget->pos().x() - 4, ui->sidebarWidget->pos().y(), 4, ui->sidebarWidget->height());
                    if (!coldArea.contains(ui->controlsWidget->mapTo(this, event->pos()))) {
                        controlsMoveStartPos = event->pos();
                        setCursor(Qt::ClosedHandCursor);
                    }
                }
            }
        } else if (ev->type() == QEvent::MouseButtonRelease) {
            if (sidebarResizeStartX >= 0)   // if resizing sidebar
                return false;
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                if (controlsMoveStartPos.x() >= 0 && controlsMoveStartPos.y() >= 0) {
                    // stop dragging controls, reset cursor
                    controlsMoveStartPos = QPoint(-1, -1);
                    unsetCursor();
                }
            }
        } else if (ev->type() == QEvent::MouseMove) {
            if (sidebarResizeStartX >= 0)   // if resizing sidebar
                return false;
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (controlsMoveStartPos.x() >= 0 && controlsMoveStartPos.y() >= 0) {
                // dragging controls panel, calculate new pos
                int newPosX = ui->controlsWidget->pos().x() + event->pos().x() - controlsMoveStartPos.x();
                newPosX = qMax(qMin(newPosX, width() - ui->controlsWidget->width()), 0);
                int newPosY = ui->controlsWidget->pos().y() + event->pos().y() - controlsMoveStartPos.y();
                newPosY = qMax(qMin(newPosY, height() - ui->controlsWidget->height()), 0);
                ui->controlsWidget->move(newPosX, newPosY);
                controlsCenterPos.setX((newPosX + ui->controlsWidget->width() * 0.5) / width());
                controlsCenterPos.setY(1. - (newPosY + ui->controlsWidget->height() * 0.5) / height());
                return true;
            }
        }
    } else if (obj == ui->mpvContainer) {
        if (ev->type() == QEvent::MouseButtonPress) {
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                // sidebar resizing has higher priority than gesture
                // only start gesture if cursor not around sidebar left border
                QRect coldArea(ui->sidebarWidget->pos().x() - 4, ui->sidebarWidget->pos().y(), 4, ui->sidebarWidget->height());
                if (!coldArea.contains(ui->mpvContainer->mapTo(this, event->pos()))) {
                    if (gestures)   // if enable gestures
                        baka->gesture->begin(GestureHandler::HSEEK_VVOLUME, event->globalPos(), pos());
                }
            } else if (event->button() == Qt::RightButton) {
                if (mpv->getPlayState() > 0) {
                    mpv->playPause();
                    return true;
                }
            }
        } else if (ev->type() == QEvent::MouseButtonRelease) {
            if (sidebarResizeStartX >= 0)   // if resizing sidebar
                return false;
#ifdef Q_OS_DARWIN
            // delayed fullscreen to work around bug
            if (delayedFullScreen >= 0) {
                fullScreen(delayedFullScreen);
                delayedFullScreen = -1;
            }
#endif
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                baka->gesture->end();
                return true;
            }
        } else if (ev->type() == QEvent::MouseMove) {
            if (sidebarResizeStartX >= 0)
                return false;
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (baka->gesture->process(event->globalPos())) {
                return false;
            }
        } else if (ev->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                // double click video area, toggle fullscreen
                if (!isFullScreen() && ui->actionFullScreen->isEnabled())
                    fullScreen(true, true);
                else
                    fullScreen(false, true);
            }
        }
    } else if (obj == ui->centralwidget) {
        if (ev->type() == QEvent::MouseButtonPress) {
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                if (ui->sidebarWidget->isVisible()) {
                    // if cursor around sidebar left border, start resizing
                    // if clicked elsewhere, hide sidebar
                    QRect hotArea(ui->sidebarWidget->pos().x() - 4, ui->sidebarWidget->pos().y(), 4, ui->sidebarWidget->height());
                    if (hotArea.contains(ui->centralwidget->mapTo(this, event->pos())))
                        sidebarResizeStartX = ui->sidebarWidget->width() + event->pos().x();
                    else if (!ui->sidebarWidget->rect().contains(ui->sidebarWidget->mapFromGlobal(event->globalPos())))
                        showSidebar(false, !isFullScreen());
                }
            }
        } else if (ev->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                if (sidebarResizeStartX >= 0)   // if resizing sidebar
                    sidebarResizeStartX = -1;
            }
        } else if (ev->type() == QEvent::MouseMove) {
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (ui->sidebarWidget->isVisible()) {
                QRect hotArea(ui->sidebarWidget->pos().x() - 4, ui->sidebarWidget->pos().y(), 4, ui->sidebarWidget->height());
                if (hotArea.contains(ui->centralwidget->mapTo(this, event->pos())))
                    setCursor(Qt::SizeHorCursor);
                else if (sidebarResizeStartX < 0)
                    unsetCursor();
                if (sidebarResizeStartX >= 0) {
                    // resizing sidebar, calculate new geometry
                    int newWidth = sidebarResizeStartX - event->pos().x();
                    newWidth = qMax(qMin(newWidth, width() / 2), 100);
                    ui->sidebarWidget->setGeometry(ui->centralwidget->width() - newWidth, 0, newWidth, ui->centralwidget->height());
                    sidebarWidth = newWidth;
                }
            }
            if (mpv->getPlayState() > 0)
                showCursorAndControls(event);
        } else if (ev->type() == QEvent::Leave) {
            unsetCursor();
        }
    }
    return false;
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (ui->sidebarWidget->rect().contains(ui->sidebarWidget->mapFromGlobal(event->globalPos())))
        return;
    if (event->delta() > 0)
        mpv->setVolume(mpv->getVolume() + 5, true);
    else
        mpv->setVolume(mpv->getVolume() - 5, true);
    QMainWindow::wheelEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // keyboard shortcuts
    if (!baka->input.empty()) {
        QString key = QKeySequence(event->modifiers() | event->key()).toString();
        key.replace("Num+", "");

        // Escape exits fullscreen
        if (isFullScreen() && key == "Esc") {
            fullScreen(false);
            return;
        }

        // find shortcut in input hash table
        auto iter = baka->input.find(key);
        if (iter != baka->input.end())
            baka->command(iter->first); // execute command
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    int width = event->size().width();
    int height = event->size().height();
    ui->mpvContainer->setGeometry(0, 0, width, height);
    ui->startupPage->setGeometry(0, 0, width, height);
    updateControlsPos();
    updateSidebarWidth();

    if (ui->actionMediaInfo->isChecked())
        baka->overlay->showInfoText();
    QMainWindow::resizeEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::setIndexLabels(bool enable)
{
    int i = ui->playlistWidget->selectedRow(),
        index = ui->playlistWidget->playingRow();

    // next file
    if (enable && index + 1 < ui->playlistWidget->count())
        enableNextButton(true);
    else
        enableNextButton(false);

    // previous file
    if (enable && index - 1 >= 0)
        enablePreviousButton(true);
    else
        enablePreviousButton(false);

    if (i == -1) { // no selection
        ui->indexLabel->setText(tr("No selection"));
        ui->indexLabel->setEnabled(false);
    } else {
        ui->indexLabel->setEnabled(true);
        ui->indexLabel->setText(tr("%0 / %1").arg(QString::number(i + 1), QString::number(ui->playlistWidget->count())));
    }
}

void MainWindow::enablePlaybackControls(bool enable)
{
    // playback controls
    ui->seekBar->setEnabled(enable);
    if (enable)
        updateBufferRange->start(1000);
    else
        updateBufferRange->stop();

    setIndexLabels(enable);

    // menubar
    ui->actionPlay->setEnabled(enable);
    ui->actionStop->setEnabled(enable);
    ui->actionRestart->setEnabled(enable);
    ui->actionStopAfterCurrent->setEnabled(enable);
    ui->menuSpeed->setEnabled(enable);
    ui->actionJumpToTime->setEnabled(enable);
    ui->actionMediaInfo->setEnabled(enable);
    ui->actionClose->setEnabled(enable);
    ui->actionShowInFolder->setEnabled(enable && baka->mpv->getPath() != QString());
    ui->actionFullScreen->setEnabled(enable);
    if (!enable) {
        ui->menuSubtitleTrack->setEnabled(false);
        ui->menuFontSize->setEnabled(false);
    }
}

void MainWindow::enableTrackOperations(bool enable)
{
    if (enable && ui->menuSubtitleTrack->actions().count() > 1) {
        ui->menuSubtitleTrack->setEnabled(true);
        ui->actionSubtitleTrackNone->setEnabled(true);
    } else
        ui->menuSubtitleTrack->setEnabled(false);
    ui->actionAddSubtitleFile->setEnabled(enable);

    if (enable && ui->menuAudioTrack->actions().count() > 1) {
        ui->menuAudioTrack->setEnabled(true);
        ui->actionAudioTrackNone->setEnabled(true);
    } else
        ui->menuAudioTrack->setEnabled(false);
    ui->actionAddAudioFile->setEnabled(enable);

    if (enable && ui->menuVideoTrack->actions().count() > 1) {
        ui->menuVideoTrack->setEnabled(true);
        ui->actionVideoTrackNone->setEnabled(true);
    } else
        ui->menuVideoTrack->setEnabled(false);
}

void MainWindow::enableAudioFunctions(bool enable)
{
    ui->menuAudioDelay->setEnabled(enable);
}

void MainWindow::enableVideoFunctions(bool enable)
{
    ui->menuSubtitleTrack->setEnabled(enable);
    ui->actionAddSubtitleFile->setEnabled(enable);
    for (auto &action : subtitlePluginActions)
        action->setEnabled(enable);

    bool enableSub = (enable && ui->menuSubtitleTrack->actions().count() > 1);
    ui->menuFontSize->setEnabled(enableSub);
    ui->actionShowSubtitles->setEnabled(enableSub);
    ui->actionShowSubtitles->setChecked(enableSub && mpv->getSubtitleVisibility());
    ui->actionSubtitleFont->setEnabled(enableSub);
    ui->menuSubtitleDelay->setEnabled(enableSub);
    ui->menuSubtitleEncoding->setEnabled(enableSub);
    ui->menuSubtitlePos->setEnabled(enableSub);
    ui->menuSubtitleStyle->setEnabled(enableSub);

    ui->menuScreenshot->setEnabled(enable);
    ui->menuVideoSize->setEnabled(enable);
    ui->actionFrameStep->setEnabled(enable);
    ui->actionFrameBackStep->setEnabled(enable);
    ui->actionDeinterlace->setEnabled(enable);
    ui->actionMotionInterpolation->setEnabled(enable);
}

void MainWindow::fullScreen(bool fullScreen, bool doubleClick)
{
    if (fullScreen && baka->dimDialog && baka->dimDialog->isVisible())
        baka->dim(false);

    // fix bug: https://bugreports.qt.io/browse/QTBUG-46077
#ifdef Q_OS_DARWIN
    if (doubleClick) {
        delayedFullScreen = fullScreen;
        return;
    }
#else
    Q_UNUSED(doubleClick);
#endif

    if (fullScreen)
        setWindowState(windowState() | Qt::WindowFullScreen);
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void MainWindow::showStartupPage(bool visible)
{
    if (visible) {
        ui->startupPage->show();
        ui->mpvContainer->hide();
        ui->controlsWidget->hide();
        autoHideControls->stop();
    } else {
        ui->startupPage->hide();
        ui->mpvContainer->show();
    }
}

void MainWindow::closeFile()
{
    if (current != nullptr) {
        double t = mpv->getTime(), l = mpv->getFileInfo().length;
        current->time = (t > 0.05 * l && t < 0.95 * l) ? t : 0;
    }
    mpv->stop();
    init = false;
}

bool MainWindow::isSidebarVisible(int index)
{
    if (index >= 0)
        return ui->sidebarWidget->isVisible() && ui->tabWidget->currentIndex() == index;

    return ui->sidebarWidget->isVisible();
}

void MainWindow::toggleSidebar(int index)
{
    showSidebar(!isSidebarVisible(index), !isFullScreen(), index);
}

void MainWindow::showSidebar(bool visible, bool anim, int index)
{
    if (index >= 0)
        ui->tabWidget->setCurrentIndex(index);

    if (visible == ui->sidebarWidget->isVisible())
        return;

    if (visible) {
        updateSidebarWidth();
        ui->sidebarWidget->show();
        ui->sidebarWidget->setFocus();
        if (anim) {
            QPropertyAnimation *a = new QPropertyAnimation(ui->sidebarWidget, "pos");
            a->setDuration(200);
            a->setStartValue(QPoint(width(), 0));
            a->setEndValue(ui->sidebarWidget->pos());
            a->start(QPropertyAnimation::DeleteWhenStopped);
        }
    } else {
        if (anim) {
            QPropertyAnimation *a = new QPropertyAnimation(ui->sidebarWidget, "pos");
            a->setDuration(200);
            a->setStartValue(ui->sidebarWidget->pos());
            a->setEndValue(QPoint(width(), 0));
            a->start(QPropertyAnimation::DeleteWhenStopped);
            connect(a, &QAbstractAnimation::finished, [=] {
                ui->sidebarWidget->hide();
                activateWindow();
                setFocus();
            });
        } else {
            ui->sidebarWidget->hide();
            activateWindow();   // fix mac setFocus bug
            setFocus();
        }
    }
}

void MainWindow::showControls(bool visible, bool anim)
{
    if (visible == ui->controlsWidget->isVisible())
        return;

    if (visible) {
        ui->controlsWidget->show();
        if (anim) {
            QGraphicsOpacityEffect *e = new QGraphicsOpacityEffect(this);
            ui->controlsWidget->setGraphicsEffect(e);
            QPropertyAnimation *a = new QPropertyAnimation(e, "opacity");
            a->setDuration(200);
            a->setStartValue(0);
            a->setEndValue(1);
            a->start(QPropertyAnimation::DeleteWhenStopped);
#ifdef ENABLE_MPV_COCOA_WIDGET
            connect(a, &QVariantAnimation::valueChanged, [=] (const QVariant &value) {
                Util::setLayerOpacity(ui->controlsWidget, value.toDouble());
            });
#endif
        }
    } else {
        if (anim) {
            QGraphicsOpacityEffect *e = new QGraphicsOpacityEffect(this);
            ui->controlsWidget->setGraphicsEffect(e);
            QPropertyAnimation *a = new QPropertyAnimation(e, "opacity");
            a->setDuration(200);
            a->setStartValue(1);
            a->setEndValue(0);
            a->start(QPropertyAnimation::DeleteWhenStopped);
            connect(a, &QAbstractAnimation::finished, [=] {
                ui->controlsWidget->hide();
            });
#ifdef ENABLE_MPV_COCOA_WIDGET
            connect(a, &QVariantAnimation::valueChanged, [=] (const QVariant &value) {
                Util::setLayerOpacity(ui->controlsWidget, value.toDouble());
            });
#endif
        } else {
            ui->controlsWidget->hide();
        }
    }
}

void MainWindow::updateRecentFiles()
{
    ui->menuRecentlyOpened->clear();
    QAction *action;
    int n = 1,
        N = recent.length();
    for (auto &f : recent) {
        action = ui->menuRecentlyOpened->addAction(QString("%0. %1").arg(Util::formatNumberWithAmpersand(n, N), Util::formatPath(f).replace("&","&&")));
        if (n++ == 1)
            action->setShortcut(QKeySequence("Ctrl+Z"));
        connect(action, &QAction::triggered, [=] {
            mpv->playFile(f.path, f.title);
        });
    }
}

void MainWindow::enableNextButton(bool enable)
{
    ui->nextButton->setEnabled(enable);
    ui->actionPlayNextFile->setEnabled(enable);
#if defined(Q_OS_WIN)
    nextToolButton->setEnabled(enable);
#endif
}

void MainWindow::enablePreviousButton(bool enable)
{
    ui->previousButton->setEnabled(enable);
    ui->actionPlayPreviousFile->setEnabled(enable);
#if defined(Q_OS_WIN)
    prevToolButton->setEnabled(enable);
#endif
}

void MainWindow::setPlayButtonIcon(bool play)
{
    if (play) {
        ui->playButton->setIcon(QIcon(":/img/default_play.svg"));
        ui->actionPlay->setText(tr("&Play"));
#if defined(Q_OS_WIN)
        playPauseToolButton->setToolTip(tr("Play"));
        playPauseToolButton->setIcon(QIcon(":/img/tool-play.ico"));
#endif
    } else { // pause icon
        ui->playButton->setIcon(QIcon(":/img/default_pause.svg"));
        ui->actionPlay->setText(tr("&Pause"));
#if defined(Q_OS_WIN)
        playPauseToolButton->setToolTip(tr("Pause"));
        playPauseToolButton->setIcon(QIcon(":/img/tool-pause.ico"));
#endif
    }
}

void MainWindow::setRemainingLabels(double time)
{
    // todo: move setVisible functions outside of this function which gets called every second and somewhere at the start of a video
    const Mpv::FileInfo &fi = mpv->getFileInfo();
    if (fi.length == 0) {
        ui->durationLabel->setText(Util::formatTime(time, time));
    } else {
        ui->durationLabel->setText(Util::formatTime(time, fi.length));
        if (remaining) {
            double remainingTime = fi.length - time;
            QString text = "-" + Util::formatTime(remainingTime, fi.length);
            if (mpv->getSpeed() != 1) {
                double speed = mpv->getSpeed();
                text += QString("  (-%0)").arg(Util::formatTime(remainingTime / speed, fi.length / speed));
            }
            ui->remainingLabel->setText(text);
        } else {
            QString text = Util::formatTime(fi.length, fi.length);
            if (mpv->getSpeed() != 1) {
                double speed = mpv->getSpeed();
                text += QString("  (%0)").arg(Util::formatTime(fi.length / speed, fi.length / speed));
            }
            ui->remainingLabel->setText(text);
        }
    }
}

void MainWindow::hideCursorAndControls()
{
    if (ui->mpvContainer->geometry().contains(ui->mpvContainer->mapFromGlobal(cursor().pos())))
        setCursor(Qt::BlankCursor);
    showControls(false);
    autoHideControls->stop();
}

void MainWindow::showCursorAndControls(QMouseEvent *event)
{
    if (cursor().shape() == Qt::BlankCursor)
        unsetCursor();
    showControls(true);
    autoHideControls->stop();
    if (sidebarResizeStartX < 0 && !isContextMenuVisible &&
        (!event || !ui->controlsWidget->rect().contains(ui->controlsWidget->mapFromGlobal(event->globalPos()))))
        autoHideControls->start(3000);
}
