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
    ui->menu_Help->insertSeparator(ui->actionAbout_Qt);
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
    ui->actionUpdate_Streaming_Support->setEnabled(false);
#endif
    addActions(ui->menubar->actions()); // makes menubar shortcuts work even when menubar is hidden

    ui->playlistSearchBox->setIcon(QIcon(":/img/search.svg"), QSize(16, 16));
    ui->openFileButton->setIcon(QIcon(":/img/default_open.svg"), QSize(16, 16), 12);
    ui->openUrlButton->setIcon(QIcon(":/img/link.svg"), QSize(16, 16), 12);
    ui->viewLibraryButton->setIcon(QIcon(":/img/library.svg"), QSize(16, 16), 12);

    ui->playlistWidget->attachEngine(baka);
    ui->libraryWidget->attachEngine(baka);
    ui->libraryWidget->setPlaceholderText(tr("No media provider selected."));

    // fix tab widget intercepting MouseMove event
    auto fixCursor = [=] (QMouseEvent *event) {
        if (sidebarResizeStartX < 0)
            unsetCursor();
        if (mpv->getPlayState() > 0)
            showCursorAndControls(event);
    };
    connect(ui->playlistWidget, &CustomListView::mouseMoved, fixCursor);
    connect(ui->libraryWidget, &CustomListView::mouseMoved, fixCursor);

    // install event filters
    ui->mpvContainer->installEventFilter(this);
    ui->controlsWidget->installEventFilter(this);
    ui->centralwidget->installEventFilter(this);

    QList<QWidget *> widgetList = findChildren<QWidget *>();
    for (auto &widget : widgetList) {
        widget->setMouseTracking(true);
    }

    // command action mappings (action (right) performs command (left))
    commandActionMap = {
        {"mpv add chapter +1", ui->action_Next_Chapter},
        {"mpv add chapter -1", ui->action_Previous_Chapter},
        {"mpv set sub-scale 1", ui->action_Reset_Size},
        {"mpv add sub-scale +0.1", ui->action_Size},
        {"mpv add sub-scale -0.1", ui->actionS_ize},
        {"mpv cycle sub-visibility", ui->actionShow_Subtitles},
        {"mpv set time-pos 0", ui->action_Restart},
        {"mpv frame_step", ui->action_Frame_Step},
        {"mpv frame_back_step", ui->actionFrame_Back_Step},
        {"deinterlace", ui->action_Deinterlace},
        {"interpolate", ui->action_Motion_Interpolation},
        {"mute", ui->action_Mute},
        {"screenshot subtitles", ui->actionWith_Subtitles},
        {"screenshot", ui->actionWithout_Subtitles},
        {"screenshot show", ui->actionOpen_Screenshot_Folder},
        {"add_subtitles", ui->action_Add_Subtitle_File},
        {"add_audio", ui->action_Add_Audio_File},
        {"video_size", ui->actionFit_Screen},
        {"video_size 50", ui->action50},
        {"video_size 75", ui->action75},
        {"video_size 100", ui->action100},
        {"video_size 150", ui->action150},
        {"video_size 200", ui->action200},
        {"fullscreen", ui->action_Full_Screen},
        {"jump", ui->action_Jump_to_Time},
        {"media_info", ui->actionMedia_Info},
        {"new", ui->action_New_Player},
        {"open", ui->action_Open_File},
        {"open_clipboard", ui->actionOpen_Path_from_Clipboard},
        {"open_location", ui->actionOpen_URL},
        {"close", ui->actionClose},
        {"playlist play +1", ui->actionPlay_Next_File},
        {"playlist play -1", ui->actionPlay_Previous_File},
        {"playlist repeat off", ui->action_Off},
        {"playlist repeat playlist", ui->action_Playlist},
        {"playlist repeat this", ui->action_This_File},
        {"playlist shuffle", ui->actionSh_uffle},
        {"playlist toggle", ui->action_Show_Playlist},
        {"library toggle", ui->action_Show_Library},
        {"dim", ui->action_Dim_Lights},
        {"play_pause", ui->action_Play},
        {"quit", ui->actionE_xit},
        {"show_in_folder", ui->actionShow_in_Folder},
        {"stop", ui->action_Stop},
        {"volume +5", ui->action_Increase_Volume},
        {"volume -5", ui->action_Decrease_Volume},
        {"audio_delay +0.5", ui->action_Increase_Audio_Delay},       // "mpv add audio-delay +0.5"
        {"audio_delay -0.5", ui->action_Decrease_Audio_Delay},       // "mpv add audio-delay -0.5"
        {"audio_delay 0", ui->action_Reset_Audio_Delay},             // "mpv set audio-delay 0"
        {"subtitle_delay +0.5", ui->action_Increase_Subtitle_Delay}, // "mpv add sub-delay +0.5"
        {"subtitle_delay -0.5", ui->action_Decrease_Subtitle_Delay}, // "mpv add sub-delay -0.5"
        {"subtitle_delay 0", ui->action_Reset_Subtitle_Delay},       // "mpv set sub-delay 0"
        {"subtitle_font", ui->action_Subtitle_Font},
        {"mpv add sub-pos -3", ui->action_Subtitle_Up},
        {"mpv add sub-pos +3", ui->action_Subtitle_Down},
        {"mpv set sub-pos 100", ui->action_Reset_Subtitle_Pos},
        {"subtitle_style color", ui->action_Subtitle_Color},
        {"subtitle_style back-color", ui->action_Subtitle_Back_Color},
        {"subtitle_style blur", ui->action_Subtitle_Blur},
        {"subtitle_style shadow-offset", ui->action_Subtitle_Shadow_Offset},
        {"subtitle_style shadow-color", ui->action_Subtitle_Shadow_Color},
        {"speed +0.1", ui->action_Increase},
        {"speed -0.1", ui->action_Decrease},
        {"speed 2.0", ui->action_Double_Speed},
        {"speed 0.5", ui->action_Half_Speed},
        {"speed 1.0", ui->action_Reset},
        {"preferences", ui->action_Preferences},
        {"online_help", ui->actionOnline_Help},
        {"update", ui->action_Check_for_Updates},
        {"update youtube-dl", ui->actionUpdate_Streaming_Support},
        {"about qt", ui->actionAbout_Qt},
        {"about", ui->actionAbout_Baka_MPlayer}
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

    // dimDialog
    connect(baka->dimDialog, &DimDialog::visbilityChanged, [=] (bool dim) {
        if (dim)
            Util::setAlwaysOnTop(this, true);
        else if (onTop == "never" || (onTop == "playing" && mpv->getPlayState() > 0))
            Util::setAlwaysOnTop(this, false);
    });

    // mpv

    connect(ui->playlistWidget, &PlaylistWidget::playlistChanged, [=] (QStandardItemModel *model) {
        if (model->rowCount() > 1) {
            ui->actionSh_uffle->setEnabled(true);
            ui->actionStop_after_Current->setEnabled(true);
        } else {
            ui->actionSh_uffle->setEnabled(false);
            ui->actionStop_after_Current->setEnabled(false);
        }

        if (model->rowCount() > 0)
            ui->menuR_epeat->setEnabled(true);
        else
            ui->menuR_epeat->setEnabled(false);
    });

    connect(mpv, &MpvHandler::fileInfoChanged, [=] (const Mpv::FileInfo &fileInfo) {
        if (mpv->getPlayState() > 0) {
            if (fileInfo.media_title == "")
                setWindowTitle("Baka MPlayer");
            else if (fileInfo.media_title == "-")
                setWindowTitle("Baka MPlayer: stdin"); // todo: disable playlist?
            else
                setWindowTitle(fileInfo.media_title);

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
                    recent.push_front(Recent(file, (mpv->getPath().isEmpty() || !Util::isValidFile(file)) ? fileInfo.media_title : QString()));
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

            if (ui->actionMedia_Info->isChecked())
                baka->mediaInfo(true);

            setRemainingLabels(fileInfo.length);
        }
    });

    connect(ui->action_none, &QAction::triggered, [=] {
        if (mpv->getSid() != 0)
            mpv->setSid(0);
        else
            ui->action_none->setChecked(true);
    });

    connect(ui->action_none_3, &QAction::triggered, [=] {
        if (mpv->getAid() != 0)
            mpv->setAid(0);
        else
            ui->action_none_3->setChecked(true);
    });

    connect(ui->action_none_5, &QAction::triggered, [=] {
        if (mpv->getVid() != 0)
            mpv->setVid(0);
        else
            ui->action_none_5->setChecked(true);
    });

    connect(mpv, &MpvHandler::trackListChanged, [=] (const QList<Mpv::Track> &trackList) {
        ui->menuSubtitle_Track->clear();
        ui->menuSubtitle_Track->addAction(ui->action_none);
        ui->menuAudio_Track->clear();
        ui->menuAudio_Track->addAction(ui->action_none_3);
        ui->menuVideo_Track->clear();
        ui->menuVideo_Track->addAction(ui->action_none_5);

        if (mpv->getPlayState() > 0) {
            QAction *action;
            QString title;
            bool hasAudio = false, hasVideo = false;
            for (auto &track : trackList) {
                if (track.type == "sub") {
                    title = mpv->formatTrackInfo(track);
                    action = ui->menuSubtitle_Track->addAction(title);
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
                    action = ui->menuAudio_Track->addAction(title);
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
                    if (!track.albumart) // isn't album art
                        hasVideo = true;
                    title = mpv->formatTrackInfo(track);
                    action = ui->menuVideo_Track->addAction(title);
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
                baka->sysTrayIcon->showMessage("Baka MPlayer", mpv->getFileInfo().media_title, QSystemTrayIcon::NoIcon, 4000);
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
            ui->menu_Chapters->clear();
            for (auto &ch : chapters) {
                action = ui->menu_Chapters->addAction(QString("%0: %1").arg(Util::formatNumberWithAmpersand(n, N), ch.title));
                if (n <= 9)
                    action->setShortcut(QKeySequence("Ctrl+" + QString::number(n)));
                connect(action, &QAction::triggered, [=] {
                    mpv->seek(ch.time);
                });
                ticks.push_back(ch.time);
                n++;
            }
            if (ui->menu_Chapters->actions().count() == 0) {
                ui->menu_Chapters->setEnabled(false);
                ui->action_Next_Chapter->setEnabled(false);
                ui->action_Previous_Chapter->setEnabled(false);
            } else {
                ui->menu_Chapters->setEnabled(true);
                ui->action_Next_Chapter->setEnabled(true);
                ui->action_Previous_Chapter->setEnabled(true);
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
                ui->action_Play->setEnabled(true);
                ui->playButton->setEnabled(true);
#if defined(Q_OS_WIN)
                playPauseToolButton->setEnabled(true);
#endif
                ui->action_Show_Playlist->setEnabled(true);
                ui->action_Show_Library->setEnabled(true);
                ui->menuSubtitle_Track->setEnabled(true);
                ui->menuAudio_Track->setEnabled(true);
                ui->menuVideo_Track->setEnabled(true);
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
            bool stop = false;
            if (init) {
                if (ui->action_This_File->isChecked()) // repeat this file
                    ui->playlistWidget->playRow(0, true); // restart file
                else if (ui->actionStop_after_Current->isChecked() ||  // stop after playing this file
                        ui->playlistWidget->playingRow() >= ui->playlistWidget->count() - 1) { // end of the playlist
                    if (!ui->actionStop_after_Current->isChecked() && // not supposed to stop after current
                        ui->action_Playlist->isChecked() && // we're supposed to restart the playlist
                        ui->playlistWidget->count() > 0) { // playlist isn't empty
                        ui->playlistWidget->playRow(0); // restart playlist
                    } else // stop
                        stop = true;
                } else
                    ui->playlistWidget->playRow(1, true);
            } else
                stop = true;

            if (stop) {
                setWindowTitle("Baka MPlayer");
                enablePlaybackControls(false);
                ui->seekBar->setTotalTime(0);
                ui->actionStop_after_Current->setChecked(false);
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
            ui->action_Next_Chapter->setEnabled(i < fi.chapters.last().time);
            ui->action_Previous_Chapter->setEnabled(i > fi.chapters.first().time);
        }
    });

    connect(mpv, &MpvHandler::volumeChanged, [=] (int volume) {
        ui->volumeSlider->setValueNoSignal(volume);
    });

    connect(mpv, &MpvHandler::speedChanged, [=] (double speed) {
        static double last = 1;
        if (last != speed) {
            if (init)
                mpv->showText(tr("Speed: %0x").arg(QString::number(speed)));
            if (speed <= 0.25)
                ui->action_Decrease->setEnabled(false);
            else
                ui->action_Decrease->setEnabled(true);
            last = speed;
        }
    });

    connect(mpv, &MpvHandler::sidChanged, [=] (int sid) {
        QList<QAction*> actions = ui->menuSubtitle_Track->actions();
        for (auto &action : actions) {
            if ((!sid && action == ui->action_none) || (sid && action->text().startsWith(QString::number(sid)))) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::aidChanged, [=] (int aid) {
        QList<QAction*> actions = ui->menuAudio_Track->actions();
        for (auto &action : actions) {
            if (mpv->getPlayState() > 0 && ((!aid && action == ui->action_none_3) ||
                                            (aid && action->text().startsWith(QString::number(aid))))) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::vidChanged, [=] (int vid) {
        QList<QAction*> actions = ui->menuVideo_Track->actions();
        for (auto &action : actions) {
            if (mpv->getPlayState() > 0 && ((!vid && action == ui->action_none_5) ||
                                            (vid && action->text().startsWith(QString::number(vid))))) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::subtitleVisibilityChanged, [=] (bool b) {
        if (ui->actionShow_Subtitles->isEnabled())
            ui->actionShow_Subtitles->setChecked(b);
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
        ui->action_Motion_Interpolation->setChecked(vo.contains("interpolation"));
    });

    connect(mpv, &MpvHandler::audioDeviceChanged, [=] (QString device) {
        QList<QAction*> actions = ui->menuAudio_Device->actions();
        for (auto &action : actions) {
            if (action->text().endsWith(device)) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::audioDeviceListChanged, [=] (const QList<Mpv::AudioDevice> &deviceList) {
        ui->menuAudio_Device->clear();
        for (const auto &device : deviceList) {
            QString title = QString("[%0] %1").arg(device.description, device.name);
            QAction *action = ui->menuAudio_Device->addAction(title);
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
        QList<QAction*> actions = ui->menuSubtitle_Encoding->actions();
        for (auto &action : actions) {
            if (action->text() == title) {
                action->setCheckable(true);
                action->setChecked(true);
            } else
                action->setChecked(false);
        }
    });

    connect(mpv, &MpvHandler::subtitleEncodingListChanged, [=] (const QList<QPair<QString, QString> > &encodingList) {
        ui->menuSubtitle_Encoding->clear();
        for (const auto &pair : encodingList) {
            QAction *action = ui->menuSubtitle_Encoding->addAction(pair.second);
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
    connect(ui->playlistWidget, &CustomListView::currentRowChanged, this, [=] (int) {
        setIndexLabels(true);
    }, Qt::QueuedConnection);

    // playlist: repeat button clicked, switch between off/single/list
    connect(ui->repeatButton, &QPushButton::clicked, [=] {
        if (ui->action_Off->isChecked())
            ui->action_Playlist->trigger();
        else if (ui->action_Playlist->isChecked())
            ui->action_This_File->trigger();
        else if (ui->action_This_File->isChecked())
            ui->action_Off->trigger();
    });

    connect(ui->shuffleButton, &QPushButton::clicked, ui->actionSh_uffle, &QAction::trigger);

    // playlist: add button clicked, add item to playlist
    connect(ui->addButton, &QPushButton::clicked, [=] {
        QString file = QFileDialog::getOpenFileName(this, tr("Add File to Playlist"), mpv->getPath(),
                                                    QString("%0 (%1);;").arg(tr("Media Files"), Mpv::media_filetypes.join(" ")) +
                                                    QString("%0 (%1);;").arg(tr("Video Files"), Mpv::video_filetypes.join(" ")) +
                                                    QString("%0 (%1);;").arg(tr("Audio Files"), Mpv::audio_filetypes.join(" ")) +
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
    connect(ui->libraryWidget, &CustomListView::scrollReachedEnd, [=] () {
        MediaProvider *provider = ui->librarySearchBox->getCurrentProvider();
        if (provider) {
            QString word = ui->librarySearchBox->getWord();
            if (word.isEmpty())
                provider->fetchNext();
        }
    });

    // add multimedia shortcuts
    ui->action_Play->setShortcuts({ui->action_Play->shortcut(), QKeySequence(Qt::Key_MediaPlay)});
    ui->action_Stop->setShortcuts({ui->action_Stop->shortcut(), QKeySequence(Qt::Key_MediaStop)});
    ui->actionPlay_Next_File->setShortcuts({ui->actionPlay_Next_File->shortcut(), QKeySequence(Qt::Key_MediaNext)});
    ui->actionPlay_Previous_File->setShortcuts({ui->actionPlay_Previous_File->shortcut(), QKeySequence(Qt::Key_MediaPrevious)});
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
                if (subtitlePluginActions.empty())
                    ui->menuSubtitles->addSeparator();

                QAction *action = new QAction(tr("Download Subtitles from \"%0\"...").arg(name), this);
                ui->menuSubtitles->addAction(action);
                subtitlePluginActions[name] = action;
                connect(action, &QAction::triggered, [=] {
                    QString word = mpv->getFileInfo().media_title;
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
                if (QAction *action = subtitlePluginActions.value(name, nullptr)) {
                    ui->menuSubtitles->removeAction(action);
                    subtitlePluginActions.remove(name);
                    action->deleteLater();
                }
                if (QAction *action = subtitlePluginActions.value(name + " Result", nullptr)) {
                    ui->menuSubtitles->removeAction(action);
                    subtitlePluginActions.remove(name + " Result");
                    action->menu()->deleteLater();
                }
                auto actions = ui->menuSubtitles->actions();
                if (subtitlePluginActions.empty() && actions.back()->isSeparator())
                    ui->menuSubtitles->removeAction(actions.back());
            }
        });

        connect(provider, &SubtitleProvider::searchFinished, this, [=] (const QList<SubtitleEntry> &result) {
            QMenu *menu = nullptr;
            QAction *act = subtitlePluginActions.value(name + " Result", nullptr);
            if (act) {
                menu = act->menu();
                menu->clear();
            } else {
                menu = new QMenu(tr("Subtitles from \"%0\"").arg(name), this);
                ui->menuSubtitles->insertMenu(ui->action_Add_Subtitle_File, menu);
                subtitlePluginActions[name + " Result"] = menu->menuAction();
            }
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

        connect(provider, &SubtitleProvider::downloadFinished, this, [=] (const SubtitleEntry &entry) {
            QString localFile = Util::toLocalFile(entry.url);
            if (!localFile.isEmpty())
                mpv->addSubtitleTrack(localFile);
            else if (Util::isValidUrl(entry.url)) {
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
            if (result.isEmpty()) {
                ui->libraryWidget->setPlaceholderText(tr("No result found."));
                ui->libraryWidget->update();
                return;
            }
            auto provider = ui->librarySearchBox->getCurrentProvider();
            for (auto &entry : result) {
                MediaEntry *e = new MediaEntry(entry);
                QPersistentModelIndex index = ui->libraryWidget->appendEntry(e);
                if (entry.cover.isNull())
                    provider->download(entry, "cover", index);
            }
        };
        connect(provider, &MediaProvider::fetchFinished, this, populateMediaList);
        connect(provider, &MediaProvider::searchFinished, this, populateMediaList);

        connect(provider, &MediaProvider::downloadFinished, this, [=] (const MediaEntry &entry, QString what, const QPersistentModelIndex &index) {
            if (what == "")
                mpv->loadFile(entry.url, entry.name, entry.options);
            else if (what == "cover") {
                MediaEntry *item = index.data(Qt::UserRole).value<MediaEntry*>();
                if (!item)
                    return;
                QString localFile = Util::toLocalFile(entry.coverUrl);
                if (!localFile.isEmpty()) {
                    item->cover.load(localFile);
                    ui->libraryWidget->update(index);
                } else if (Util::isValidUrl(entry.coverUrl)) {
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
    mpv->loadFile(file);
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

Mpv::PlaylistItem *MainWindow::getCurrentPlayFile()
{
    return ui->playlistWidget->currentItem();
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
            if (url.isLocalFile())
                mpv->loadFile(url.toLocalFile());
            else
                mpv->loadFile(url.url());
        }
    } else if (mimeData->hasText()) // text
        mpv->loadFile(mimeData->text());
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
                if (ui->remainingLabel->rect().contains(ui->remainingLabel->mapFrom(ui->controlsWidget, event->pos()))) { // clicked timeLayoutWidget
                    setRemaining(!remaining); // todo: use a bakacommand
                    return true;
                } else {
                    QRect coldArea(ui->sidebarWidget->pos().x() - 4, ui->sidebarWidget->pos().y(), 4, ui->sidebarWidget->height());
                    if (!coldArea.contains(ui->controlsWidget->mapTo(this, event->pos()))) {
                        controlsMoveStartPos = event->pos();
                        setCursor(Qt::ClosedHandCursor);
                    }
                }
            }
        } else if (ev->type() == QEvent::MouseButtonRelease) {
            if (sidebarResizeStartX >= 0)
                return false;
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (event->button() == Qt::LeftButton) {
                if (controlsMoveStartPos.x() >= 0 && controlsMoveStartPos.y() >= 0) {
                    controlsMoveStartPos = QPoint(-1, -1);
                    unsetCursor();
                }
            }
        } else if (ev->type() == QEvent::MouseMove) {
            if (sidebarResizeStartX >= 0)
                return false;
            QMouseEvent *event = static_cast<QMouseEvent*>(ev);
            if (controlsMoveStartPos.x() >= 0 && controlsMoveStartPos.y() >= 0) {
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
                QRect coldArea(ui->sidebarWidget->pos().x() - 4, ui->sidebarWidget->pos().y(), 4, ui->sidebarWidget->height());
                if (!coldArea.contains(ui->mpvContainer->mapTo(this, event->pos()))) {
                    if (gestures)
                        baka->gesture->begin(GestureHandler::HSEEK_VVOLUME, event->globalPos(), pos());
                }
            } else if (event->button() == Qt::RightButton) {
                if (!isFullScreen() && mpv->getPlayState() > 0) {
                    mpv->playPause();
                    return true;
                }
            }
        }  else if (ev->type() == QEvent::MouseButtonRelease) {
            if (sidebarResizeStartX >= 0)
                return false;
#ifdef Q_OS_DARWIN
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
                if (!isFullScreen() && ui->action_Full_Screen->isEnabled())
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
                if (sidebarResizeStartX >= 0)
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
        mpv->setVolume(mpv->getVolume()+5, true);
    else
        mpv->setVolume(mpv->getVolume()-5, true);
    QMainWindow::wheelEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // keyboard shortcuts
    if (!baka->input.empty()) {
        QString key = QKeySequence(event->modifiers()|event->key()).toString();
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

    if (ui->actionMedia_Info->isChecked())
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
        ui->indexLabel->setText(tr("%0 / %1").arg(QString::number(i+1), QString::number(ui->playlistWidget->count())));
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
    ui->action_Play->setEnabled(enable);
    ui->action_Stop->setEnabled(enable);
    ui->action_Restart->setEnabled(enable);
    ui->actionStop_after_Current->setEnabled(enable);
    ui->menuS_peed->setEnabled(enable);
    ui->action_Jump_to_Time->setEnabled(enable);
    ui->actionMedia_Info->setEnabled(enable);
    ui->actionClose->setEnabled(enable);
    ui->actionShow_in_Folder->setEnabled(enable && baka->mpv->getPath() != QString());
    ui->action_Full_Screen->setEnabled(enable);
    if (!enable) {
        ui->menuSubtitle_Track->setEnabled(false);
        ui->menuFont_Si_ze->setEnabled(false);
    }
}

void MainWindow::enableTrackOperations(bool enable)
{
    if (enable && ui->menuSubtitle_Track->actions().count() > 1) {
        ui->menuSubtitle_Track->setEnabled(true);
        ui->action_none->setEnabled(true);
    } else
        ui->menuSubtitle_Track->setEnabled(false);
    ui->action_Add_Subtitle_File->setEnabled(enable);

    if (enable && ui->menuAudio_Track->actions().count() > 1) {
        ui->menuAudio_Track->setEnabled(true);
        ui->action_none_3->setEnabled(true);
    } else
        ui->menuAudio_Track->setEnabled(false);
    ui->action_Add_Audio_File->setEnabled(enable);

    if (enable && ui->menuVideo_Track->actions().count() > 1) {
        ui->menuVideo_Track->setEnabled(true);
        ui->action_none_5->setEnabled(true);
    } else
        ui->menuVideo_Track->setEnabled(false);
}

void MainWindow::enableAudioFunctions(bool enable)
{
    ui->menuAudio_Delay->setEnabled(enable);
}

void MainWindow::enableVideoFunctions(bool enable)
{
    ui->menuSubtitle_Track->setEnabled(enable);
    ui->action_Add_Subtitle_File->setEnabled(enable);
    for (auto &action : subtitlePluginActions)
        action->setEnabled(enable);

    bool enableSub = (enable && ui->menuSubtitle_Track->actions().count() > 1);
    ui->menuFont_Si_ze->setEnabled(enableSub);
    ui->actionShow_Subtitles->setEnabled(enableSub);
    ui->actionShow_Subtitles->setChecked(enableSub && mpv->getSubtitleVisibility());
    ui->action_Subtitle_Font->setEnabled(enableSub);
    ui->menuSubtitle_Delay->setEnabled(enableSub);
    ui->menuSubtitle_Encoding->setEnabled(enableSub);
    ui->menuSubtitle_Pos->setEnabled(enableSub);
    ui->menuSubtitle_Style->setEnabled(enableSub);

    ui->menuScreenshot->setEnabled(enable);
    ui->menuVideo_Size->setEnabled(enable);
    ui->action_Frame_Step->setEnabled(enable);
    ui->actionFrame_Back_Step->setEnabled(enable);
    ui->action_Deinterlace->setEnabled(enable);
    ui->action_Motion_Interpolation->setEnabled(enable);
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
    ui->menu_Recently_Opened->clear();
    QAction *action;
    int n = 1,
        N = recent.length();
    for (auto &f : recent) {
        action = ui->menu_Recently_Opened->addAction(QString("%0. %1").arg(Util::formatNumberWithAmpersand(n, N), Util::shortenPathToParent(f).replace("&","&&")));
        if (n++ == 1)
            action->setShortcut(QKeySequence("Ctrl+Z"));
        connect(action, &QAction::triggered, [=] {
            mpv->loadFile(f);
        });
    }
}

void MainWindow::enableNextButton(bool enable)
{
    ui->nextButton->setEnabled(enable);
    ui->actionPlay_Next_File->setEnabled(enable);
#if defined(Q_OS_WIN)
    nextToolButton->setEnabled(enable);
#endif
}

void MainWindow::enablePreviousButton(bool enable)
{
    ui->previousButton->setEnabled(enable);
    ui->actionPlay_Previous_File->setEnabled(enable);
#if defined(Q_OS_WIN)
    prevToolButton->setEnabled(enable);
#endif
}

void MainWindow::setPlayButtonIcon(bool play)
{
    if (play) {
        ui->playButton->setIcon(QIcon(":/img/default_play.svg"));
        ui->action_Play->setText(tr("&Play"));
#if defined(Q_OS_WIN)
        playPauseToolButton->setToolTip(tr("Play"));
        playPauseToolButton->setIcon(QIcon(":/img/tool-play.ico"));
#endif
    } else { // pause icon
        ui->playButton->setIcon(QIcon(":/img/default_pause.svg"));
        ui->action_Play->setText(tr("&Pause"));
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
    if (!ui->controlsWidget->rect().contains(ui->controlsWidget->mapFromGlobal(event->globalPos())) && sidebarResizeStartX < 0)
        autoHideControls->start(3000);
}
