#include "bakaengine.h"

#include <QApplication>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QProcess>
#include <QDir>
#include <QClipboard>
#include <QMessageBox>

#include "ui/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/aboutdialog.h"
#include "ui/locationdialog.h"
#include "ui/jumpdialog.h"
#include "ui/updatedialog.h"
#include "ui/screenshotdialog.h"
#include "ui/inputdialog.h"
#include "ui/preferencesdialog.h"
#include "widgets/dimdialog.h"
#include "mpvhandler.h"
#include "overlayhandler.h"
#include "updatemanager.h"
#include "util.h"


void BakaEngine::bakaMpv(QStringList &args)
{
    if (!args.empty())
        mpv->command(args);
    else
        requiresParameters("mpv");
}

void BakaEngine::bakaSh(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        QProcess *p = new QProcess(this);
        p->start(arg, args);
        connect(p, &QProcess::readyRead, [=] {
            print(p->readAll(), QString("%0(%1))").arg(p->program(), QString::number(quintptr(p))));
        });
//        connect(p, &QProcess::finished, [=] (int, QProcess::ExitStatus) {
//            delete p;
//        });
    } else
        requiresParameters("mpv");
}

void BakaEngine::bakaNew(QStringList &args)
{
    if (args.empty())
        QProcess::startDetached(QApplication::applicationFilePath(), {});
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaOpenLocation(QStringList &args)
{
    if (args.empty())
        openLocation();
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::openLocation()
{
    mpv->loadFile(LocationDialog::getUrl(mpv->getPath() + mpv->getFile(), window));
}


void BakaEngine::bakaOpenClipboard(QStringList &args)
{
    if (args.empty())
        mpv->loadFile(QApplication::clipboard()->text());
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaClose(QStringList &args)
{
    if (args.empty()) {
        window->closeFile();
    } else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaShowInFolder(QStringList &args)
{
    if (args.empty())
        Util::showInFolder(mpv->getPath(), mpv->getFile());
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaAddSubtitles(QStringList &args)
{
    QString trackFile;
    if (args.empty()) {
        trackFile = QFileDialog::getOpenFileName(window, tr("Open Subtitle File"), mpv->getPath(),
                                                 QString("%0 (%1)").arg(tr("Subtitle Files"), Mpv::subtitle_filetypes.join(" ")),
                                                 0, QFileDialog::DontUseSheet);
    } else
        trackFile = args.join(' ');

    mpv->addSubtitleTrack(trackFile);
}

void BakaEngine::bakaAddAudio(QStringList &args)
{
    QString trackFile;
    if (args.empty()) {
        trackFile = QFileDialog::getOpenFileName(window, tr("Open Audio File"), mpv->getPath(),
                                                 QString("%0 (%1)").arg(tr("Audio Files"), Mpv::audio_filetypes.join(" ")),
                                                 0, QFileDialog::DontUseSheet);
    } else
        trackFile = args.join(' ');

    mpv->addAudioTrack(trackFile);
}

void BakaEngine::bakaScreenshot(QStringList &args)
{
    if (args.empty())
        screenshot(false);
    else {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            if (arg == "subtitles")
                screenshot(true);
            else if (arg == "show")
                Util::showInFolder(mpv->getScreenshotDir(), "");
            else
                invalidParameter(arg);
        else
            invalidParameter(arg);
    }
}

void BakaEngine::screenshot(bool subs)
{
    if (window->screenshotDialog) {
        mpv->pause();
        if (ScreenshotDialog::showScreenshotDialog(window->screenshotDialog, subs, mpv) != QDialog::Accepted)
            return;
    } else
        mpv->screenshot(subs);

    QString dir = mpv->getScreenshotDir();
    int i = dir.lastIndexOf('/');
    if (i == dir.length()-1) {
        dir.remove(i, 1);
        i = dir.lastIndexOf('/');
    }
    if (i != -1)
        dir.remove(0, i+1);
    if (subs)
        mpv->showText(tr("Saved to \"%0\", with subs").arg(dir));
    else
        mpv->showText(tr("Saved to \"%0\", without subs").arg(dir));
}


void BakaEngine::bakaMediaInfo(QStringList &args)
{
    if (args.empty())
        mediaInfo(window->ui->actionMediaInfo->isChecked());
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::mediaInfo(bool show)
{
    overlay->showInfoText(show);
}


void BakaEngine::bakaStop(QStringList &args)
{
    if (args.empty())
        mpv->restartPaused();
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaPlaylist(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (arg == "play") {
            if (args.empty())
                window->ui->playlistWidget->playRow(window->ui->playlistWidget->selectedRow());
            else {
                arg = args.front();
                args.pop_front();
                if (args.empty()) {
                    if (arg.startsWith('+') || arg.startsWith('-'))
                        window->ui->playlistWidget->playRow(arg.toInt(), true);
                    else
                        window->ui->playlistWidget->playRow(arg.toInt());
                } else
                    invalidParameter(args.join(' '));
            }
        } else if (arg == "select") {
            if (args.empty())
                window->ui->playlistWidget->selectRow(window->ui->playlistWidget->playingRow());
            else {
                arg = args.front();
                args.pop_front();
                if (args.empty()) {
                    if (arg.startsWith('+') || arg.startsWith('-'))
                        window->ui->playlistWidget->selectRow(arg.toInt(), true);
                    else
                        window->ui->playlistWidget->selectRow(arg.toInt());
                } else
                    invalidParameter(args.join(' '));
            }
        } else if (args.empty()) {
            if (arg == "remove") {
                if (window->isSidebarVisible(0) && !window->ui->playlistSearchBox->hasFocus())
                    window->ui->playlistWidget->removeRow(window->ui->playlistWidget->selectedRow());
            } else if (arg == "shuffle")
                window->ui->playlistWidget->shuffle();
            else if (arg == "toggle")
                window->toggleSidebar(0);
            else
                invalidParameter(arg);
        } else if (arg == "repeat") {
            arg = args.front();
            args.pop_front();
            if (args.empty()) {
                if (arg == "off") {
                    if (window->ui->actionRepeatOff->isChecked()) {
                        window->ui->actionRepeatThisFile->setChecked(false);
                        window->ui->actionRepeatPlaylist->setChecked(false);
                        window->ui->repeatButton->setIcon(QIcon(":/img/repeat_off.svg"));
                    }
                } else if (arg == "this") {
                    if (window->ui->actionRepeatThisFile->isChecked()) {
                        window->ui->actionRepeatOff->setChecked(false);
                        window->ui->actionRepeatPlaylist->setChecked(false);
                        window->ui->repeatButton->setIcon(QIcon(":/img/repeat_one.svg"));
                    }
                } else if (arg == "playlist") {
                    if (window->ui->actionRepeatPlaylist->isChecked()) {
                        window->ui->actionRepeatOff->setChecked(false);
                        window->ui->actionRepeatThisFile->setChecked(false);
                        window->ui->repeatButton->setIcon(QIcon(":/img/repeat.svg"));
                    }
                } else
                    invalidParameter(arg);
            } else
                invalidParameter(args.join(' '));
        } else
            invalidParameter(arg);
    } else
        requiresParameters("baka playlist");
}

void BakaEngine::bakaLibrary(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            window->toggleSidebar(1);
        else
            invalidParameter(args.join(' '));
    } else
        requiresParameters("baka library");
}

void BakaEngine::bakaJump(QStringList &args)
{
    if (args.empty())
        jump();
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::jump()
{
    int time = JumpDialog::getTime(mpv->getFileInfo().length, window);
    if (time >= 0)
        mpv->seek(time);
}


void BakaEngine::bakaDim(QStringList &args)
{
    if (dimDialog == nullptr) {
        print(tr("DimDialog not supported on this platform"));
        return;
    }
    if (args.empty())
        dim(!dimDialog->isVisible());
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::dim(bool dim)
{
    if (dimDialog == nullptr) {
        QMessageBox::information(window, tr("Dim Lights"), tr("In order to dim the lights, the desktop compositor has to be enabled. This can be done through Window Manager Desktop."));
        return;
    }
    if (dim)
        dimDialog->show();
    else
        dimDialog->close();
}

void BakaEngine::bakaPreferences(QStringList &args)
{
    if (args.empty())
        PreferencesDialog::showPreferences(this, window);
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaOnlineHelp(QStringList &args)
{
    if (args.empty())
        QDesktopServices::openUrl(QUrl(tr("http://bakamplayer.u8sand.net/help.php")));
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaUpdate(QStringList &args)
{
    if (args.empty())
        UpdateDialog::checkForUpdates(this, window);
    else {
#if defined(Q_OS_WIN)
        QString arg = args.front();
        args.pop_front();
        if (arg == "youtube-dl")
            QProcess::startDetached("youtube-dl.exe", {"--update"});
        else
#endif
            invalidParameter(args.join(' '));
    }
}

void BakaEngine::bakaOpen(QStringList &args)
{
    if (args.empty())
        open();
    else
        mpv->loadFile(args.join(' '));
}

void BakaEngine::open()
{
    mpv->loadFile(QFileDialog::getOpenFileName(window,
                   tr("Open File"),mpv->getPath(),
                   QString("%0 (%1);;").arg(tr("Media Files"), Mpv::media_filetypes.join(" "))+
                   QString("%0 (%1);;").arg(tr("Video Files"), Mpv::video_filetypes.join(" "))+
                   QString("%0 (%1);;").arg(tr("Audio Files"), Mpv::audio_filetypes.join(" "))+
                   QString("%0 (*.*)").arg(tr("All Files")),
                   0, QFileDialog::DontUseSheet));
}


void BakaEngine::bakaPlayPause(QStringList &args)
{
    if (args.empty())
        playPause();
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::playPause()
{
    mpv->playPause();
}

void BakaEngine::bakaVideoSize(QStringList &args)
{
    if (args.empty())
        fitWindow();
    else {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            fitWindow(arg.toInt());
        else
            invalidParameter(args.join(' '));
    }
}

void BakaEngine::fitWindow(int percent, bool msg)
{
    if (window->isFullScreen() || window->isMaximized())
        return;

    const Mpv::VideoParams &vG = mpv->getFileInfo().videoParams; // video geometry
    QRect mG = window->ui->mpvContainer->geometry(),                      // mpv geometry
          wfG = window->frameGeometry(),                          // frame geometry of window (window geometry + window frame)
          wG = window->geometry(),                                // window geometry
          aG = qApp->desktop()->availableGeometry(wfG.center());  // available geometry of the screen we're in--(geometry not including the taskbar)

    double a, // aspect ratio
           w, // width of vid we want
           h; // height of vid we want

    // obtain natural video aspect ratio
    if (vG.width == 0 || vG.height == 0) // width/height are 0 when there is no output
        return;
    if (vG.dwidth == 0 || vG.dheight == 0) // dwidth/height are 0 on load
        a = double(vG.width)/vG.height; // use video width and height for aspect ratio
    else
        a = double(vG.dwidth)/vG.dheight; // use display width and height for aspect ratio

    // calculate resulting display:
    if (percent == 0) {
        double w1 = aG.width() - (wfG.width() - mG.width()), h1 = w1 / a;
        double h2 = aG.height() - (wfG.height() - mG.height()), w2 = h2 * a;
        w = qMin(w1, w2);
        h = qMin(h1, h2);
    } else {
        double scale = percent / 100.0; // get scale
        w = vG.width * scale;  // get scaled width
        h = vG.height * scale; // get scaled height
    }

    double dW = w + (wfG.width() - mG.width()),   // calculate display width of the window
           dH = h + (wfG.height() - mG.height()); // calculate display height of the window

    if (dW > aG.width()) {  // if the width is bigger than the available area
        dW = aG.width(); // set the width equal to the available area
        w = dW - (wfG.width() - mG.width());    // calculate the width
        h = w / a;                              // calculate height
        dH = h + (wfG.height() - mG.height());  // calculate new display height
    }
    if (dH > aG.height()) { // if the height is bigger than the available area
        dH = aG.height(); // set the height equal to the available area
        h = dH - (wfG.height() - mG.height()); // calculate the height
        w = h * a;                             // calculate the width accordingly
        dW = w + (wfG.width() - mG.width());   // calculate new display width
    }

    // get the centered rectangle we want
    QRect rect = QStyle::alignedRect(Qt::LeftToRight,
                                     Qt::AlignCenter,
                                     QSize(dW,
                                           dH),
                                     aG); // center on our screen

    // adjust the rect to compensate for the frame
    rect.setLeft(rect.left() + (wG.left() - wfG.left()));
    rect.setTop(rect.top() + (wG.top() - wfG.top()));
    rect.setRight(rect.right() - (wfG.right() - wG.right()));
    rect.setBottom(rect.bottom() - (wfG.bottom() - wG.bottom()));

    // finally set the geometry of the window
    window->setGeometry(rect);

    int gcd = Util::gcd(vG.width, vG.height);
    if (gcd)
        Util::setAspectRatio(window, vG.width / gcd, vG.height / gcd);

    if(msg)
        mpv->showText(tr("Window Size: %0").arg(percent == 0 ? tr("Fit to Screen") : (QString::number(percent)+"%")));
}

void BakaEngine::bakaDeinterlace(QStringList &args)
{
    if (args.empty())
        mpv->setDeinterlace(window->ui->actionDeinterlace->isChecked());
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaInterpolate(QStringList &args)
{
    if (args.empty())
        mpv->setInterpolate(window->ui->actionMotionInterpolation->isChecked());
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaMute(QStringList &args)
{
    if (args.empty())
        mpv->setMute(!mpv->getMute());
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaVolume(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->setVolume(mpv->getVolume()+arg.toInt(), true);
            else
                mpv->setVolume(arg.toInt(), true);
        } else
            invalidParameter(args.join(' '));
    } else
        requiresParameters("volume");
}

void BakaEngine::bakaAudioDelay(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->setAudioDelay(mpv->getAudioDelay()+arg.toDouble());
            else
                mpv->setAudioDelay(arg.toDouble());
            mpv->showText(tr("Audio Delay: %0").arg(QString::number(mpv->getAudioDelay(), 'f', 1)));
        } else
            invalidParameter(args.join(' '));
    } else
        requiresParameters("audio_delay");
}

void BakaEngine::bakaSubtitleDelay(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->setSubtitleDelay(mpv->getSubtitleDelay()+arg.toDouble());
            else
                mpv->setSubtitleDelay(arg.toDouble());
            mpv->showText(tr("Subtitle Delay: %0").arg(QString::number(mpv->getSubtitleDelay(), 'f', 1)));
        } else
            invalidParameter(args.join(' '));
    } else
        requiresParameters("subtitle_delay");
}

void BakaEngine::bakaSubtitleFont(QStringList &args)
{
    if (args.empty()) {
        bool ok = false;
        QFont font = QFontDialog::getFont(&ok, mpv->getSubtitleFont(), window, tr("Set Subtitle Font"),
                                          QFontDialog::DontUseNativeDialog);
        if (ok)
            mpv->setSubtitleFont(font);
    } else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaSubtitleStyle(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg == "color") {
                QColor color = QColorDialog::getColor(mpv->getSubtitleColor(), window, tr("Set Subtitle Color"),
                                                      QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
                if (color.isValid())
                    mpv->setSubtitleColor(color);
            } else if (arg == "back-color") {
                QColor color = QColorDialog::getColor(mpv->getSubtitleBackColor(), window, tr("Set Subtitle Background Color"),
                                                      QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
                if (color.isValid())
                    mpv->setSubtitleBackColor(color);
            } else if (arg == "blur") {
                QString input = InputDialog::getInput(tr("Input Blur Factor (0-20.0):"), tr("Set Blur Factor"), [=] (QString input) {
                    double v = input.toDouble();
                    return v >= 0 && v <= 20.0;
                }, window);
                if (!input.isEmpty()) {
                    double factor = input.toDouble();
                    mpv->setSubtitleBlur(factor);
                    window->ui->actionSubtitleBlur->setChecked(factor);
                }
            } else if (arg == "shadow-offset") {
                QString input = InputDialog::getInput(tr("Input Offset Value (0-20):"), tr("Set Shadow Offset"), [=] (QString input) {
                    int v = input.toInt();
                    return v >= 0 && v <= 20;
                }, window);
                if (!input.isEmpty()) {
                    int offset = input.toInt();
                    mpv->setSubtitleShadowOffset(offset);
                    window->ui->actionSubtitleShadowOffset->setChecked(offset);
                }
            } else if (arg == "shadow-color") {
                QColor color = QColorDialog::getColor(mpv->getSubtitleShadowColor(), window, tr("Set Shadow Color"),
                                                      QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
                if (color.isValid())
                    mpv->setSubtitleShadowColor(color);
            } else
                invalidParameter(arg);
        } else
            invalidParameter(args.join(' '));
    } else
        requiresParameters("subtitle_style");
}

void BakaEngine::bakaSpeed(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->setSpeed(mpv->getSpeed()+arg.toDouble());
            else
                mpv->setSpeed(arg.toDouble());
            mpv->showText(tr("Speed: %0x").arg(QString::number(mpv->getSpeed(), 'f', 2)));
        } else
            invalidParameter(args.join(' '));
    } else
        requiresParameters("speed");
}

void BakaEngine::bakaFullScreen(QStringList &args)
{
    if (args.empty()) {
        window->fullScreen(!window->isFullScreen());
        if (window->isFullScreen())
            mpv->showText(tr("Press ESC or double-click to leave full screen"));
    } else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaBoss(QStringList &args)
{
    if (args.empty()) {
        mpv->pause();
        window->setWindowState(window->windowState() | Qt::WindowMinimized); // minimize window
    } else
        invalidParameter(args.join(' '));
}

void BakaEngine::bakaHelp(QStringList &args)
{
    if (args.empty()) {
        print(tr("usage: baka <command> [...]"));
        print(tr("commands:"));
        int len, max_len = 22;
        for (auto command = BakaCommandMap.begin(); command != BakaCommandMap.end(); ++command) {
            QString str = QString("  %0 %1").arg(command.key(), command->second[0]);
            len = str.length();
            while (len++ <= max_len)
                str += ' ';
            str += command->second[1];
            print(str);
        }
    } else {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            auto command = BakaCommandMap.find(arg);
            if (command != BakaCommandMap.end()) { //found
                print(tr("usage: %0 %1").arg(arg, command->second[0]));
                print(tr("description:"));
                print(QString("  %0").arg(command->second[1]));
                if (command->second.length() > 2 && command->second[2] != QString()) {
                    print(tr("advanced:"));
                    print(QString("  %0").arg(command->second[2]));
                }
            } else
                invalidParameter(arg);
        } else
            invalidParameter(args.join(' '));
    }
}

void BakaEngine::bakaAbout(QStringList &args)
{
    about(args.join(' '));
}

void BakaEngine::bakaMsgLevel(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            mpv->setMsgLevel(arg);
        else
            invalidParameter(args.join(' '));
    } else
        requiresParameters("msg_level");
}

void BakaEngine::about(QString what)
{
    if (what == QString())
        AboutDialog::about(APP_VERSION, window);
    else if (what == "qt")
        qApp->aboutQt();
    else
        invalidParameter(what);
}


void BakaEngine::bakaQuit(QStringList &args)
{
    if (args.empty())
        quit();
    else
        invalidParameter(args.join(' '));
}

void BakaEngine::quit()
{
    qApp->quit();
}
