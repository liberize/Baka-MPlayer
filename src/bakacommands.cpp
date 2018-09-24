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


void BakaEngine::BakaMpv(QStringList &args)
{
    if (!args.empty())
        mpv->Command(args);
    else
        RequiresParameters("mpv");
}

void BakaEngine::BakaSh(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        QProcess *p = new QProcess(this);
        p->start(arg, args);
        connect(p, &QProcess::readyRead, [=] {
            Print(p->readAll(), QString("%0(%1))").arg(p->program(), QString::number(quintptr(p))));
        });
//        connect(p, &QProcess::finished, [=] (int, QProcess::ExitStatus) {
//            delete p;
//        });
    } else
        RequiresParameters("mpv");
}

void BakaEngine::BakaNew(QStringList &args)
{
    if (args.empty())
        QProcess::startDetached(QApplication::applicationFilePath(), {});
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaOpenLocation(QStringList &args)
{
    if (args.empty())
        OpenLocation();
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::OpenLocation()
{
    mpv->LoadFile(LocationDialog::getUrl(mpv->getPath() + mpv->getFile(), window));
}


void BakaEngine::BakaOpenClipboard(QStringList &args)
{
    if (args.empty())
        mpv->LoadFile(QApplication::clipboard()->text());
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaClose(QStringList &args)
{
    if (args.empty()) {
        window->CloseFile();
    } else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaShowInFolder(QStringList &args)
{
    if (args.empty())
        Util::ShowInFolder(mpv->getPath(), mpv->getFile());
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaAddSubtitles(QStringList &args)
{
    QString trackFile;
    if (args.empty()) {
        trackFile = QFileDialog::getOpenFileName(window, tr("Open Subtitle File"), mpv->getPath(),
                                                 QString("%0 (%1)").arg(tr("Subtitle Files"), Mpv::subtitle_filetypes.join(" ")),
                                                 0, QFileDialog::DontUseSheet);
    } else
        trackFile = args.join(' ');

    mpv->AddSubtitleTrack(trackFile);
}

void BakaEngine::BakaAddAudio(QStringList &args)
{
    QString trackFile;
    if (args.empty()) {
        trackFile = QFileDialog::getOpenFileName(window, tr("Open Audio File"), mpv->getPath(),
                                                 QString("%0 (%1)").arg(tr("Audio Files"), Mpv::audio_filetypes.join(" ")),
                                                 0, QFileDialog::DontUseSheet);
    } else
        trackFile = args.join(' ');

    mpv->AddAudioTrack(trackFile);
}

void BakaEngine::BakaScreenshot(QStringList &args)
{
    if (args.empty())
        Screenshot(false);
    else {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            if (arg == "subtitles")
                Screenshot(true);
            else if (arg == "show")
                Util::ShowInFolder(mpv->getScreenshotDir(), "");
            else
                InvalidParameter(arg);
        else
            InvalidParameter(arg);
    }
}

void BakaEngine::Screenshot(bool subs)
{
    if (window->screenshotDialog) {
        mpv->Pause();
        if (ScreenshotDialog::showScreenshotDialog(window->screenshotDialog, subs, mpv) != QDialog::Accepted)
            return;
    } else
        mpv->Screenshot(subs);

    QString dir = mpv->getScreenshotDir();
    int i = dir.lastIndexOf('/');
    if (i == dir.length()-1) {
        dir.remove(i, 1);
        i = dir.lastIndexOf('/');
    }
    if (i != -1)
        dir.remove(0, i+1);
    if (subs)
        mpv->ShowText(tr("Saved to \"%0\", with subs").arg(dir));
    else
        mpv->ShowText(tr("Saved to \"%0\", without subs").arg(dir));
}


void BakaEngine::BakaMediaInfo(QStringList &args)
{
    if (args.empty())
        MediaInfo(window->ui->actionMedia_Info->isChecked());
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::MediaInfo(bool show)
{
    overlay->showInfoText(show);
}


void BakaEngine::BakaStop(QStringList &args)
{
    if (args.empty())
        mpv->RestartPaused();
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaPlaylist(QStringList &args)
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
                    InvalidParameter(args.join(' '));
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
                    InvalidParameter(args.join(' '));
            }
        } else if (args.empty()) {
            if (arg == "remove") {
                if (window->isSidebarVisible(0) && !window->ui->playlistSearchBox->hasFocus())
                    window->ui->playlistWidget->removeRow(window->ui->playlistWidget->selectedRow());
            } else if (arg == "shuffle")
                window->ui->playlistWidget->shuffle();
            else if (arg == "toggle")
                window->ToggleSidebar(0);
            else
                InvalidParameter(arg);
        } else if (arg == "repeat") {
            arg = args.front();
            args.pop_front();
            if (args.empty()) {
                if (arg == "off") {
                    if (window->ui->action_Off->isChecked()) {
                        window->ui->action_This_File->setChecked(false);
                        window->ui->action_Playlist->setChecked(false);
                        window->ui->repeatButton->setIcon(QIcon(":/img/repeat_off.svg"));
                    }
                } else if (arg == "this") {
                    if (window->ui->action_This_File->isChecked()) {
                        window->ui->action_Off->setChecked(false);
                        window->ui->action_Playlist->setChecked(false);
                        window->ui->repeatButton->setIcon(QIcon(":/img/repeat_one.svg"));
                    }
                } else if (arg == "playlist") {
                    if (window->ui->action_Playlist->isChecked()) {
                        window->ui->action_Off->setChecked(false);
                        window->ui->action_This_File->setChecked(false);
                        window->ui->repeatButton->setIcon(QIcon(":/img/repeat.svg"));
                    }
                } else
                    InvalidParameter(arg);
            } else
                InvalidParameter(args.join(' '));
        } else
            InvalidParameter(arg);
    } else
        RequiresParameters("baka playlist");
}

void BakaEngine::BakaOnline(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            window->ToggleSidebar(1);
        else
            InvalidParameter(args.join(' '));
    } else
        RequiresParameters("baka online");
}

void BakaEngine::BakaJump(QStringList &args)
{
    if (args.empty())
        Jump();
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::Jump()
{
    int time = JumpDialog::getTime(mpv->getFileInfo().length, window);
    if (time >= 0)
        mpv->Seek(time);
}


void BakaEngine::BakaDim(QStringList &args)
{
    if (dimDialog == nullptr) {
        Print(tr("DimDialog not supported on this platform"));
        return;
    }
    if (args.empty())
        Dim(!dimDialog->isVisible());
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::Dim(bool dim)
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

void BakaEngine::BakaPreferences(QStringList &args)
{
    if (args.empty())
        PreferencesDialog::showPreferences(this, window);
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaOnlineHelp(QStringList &args)
{
    if (args.empty())
        QDesktopServices::openUrl(QUrl(tr("http://bakamplayer.u8sand.net/help.php")));
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaUpdate(QStringList &args)
{
    if (args.empty())
        UpdateDialog::CheckForUpdates(this, window);
    else {
#if defined(Q_OS_WIN)
        QString arg = args.front();
        args.pop_front();
        if (arg == "youtube-dl")
            QProcess::startDetached("youtube-dl.exe", {"--update"});
        else
#endif
            InvalidParameter(args.join(' '));
    }
}

void BakaEngine::BakaOpen(QStringList &args)
{
    if (args.empty())
        Open();
    else
        mpv->LoadFile(args.join(' '));
}

void BakaEngine::Open()
{
    mpv->LoadFile(QFileDialog::getOpenFileName(window,
                   tr("Open File"),mpv->getPath(),
                   QString("%0 (%1);;").arg(tr("Media Files"), Mpv::media_filetypes.join(" "))+
                   QString("%0 (%1);;").arg(tr("Video Files"), Mpv::video_filetypes.join(" "))+
                   QString("%0 (%1);;").arg(tr("Audio Files"), Mpv::audio_filetypes.join(" "))+
                   QString("%0 (*.*)").arg(tr("All Files")),
                   0, QFileDialog::DontUseSheet));
}


void BakaEngine::BakaPlayPause(QStringList &args)
{
    if (args.empty())
        PlayPause();
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::PlayPause()
{
    mpv->PlayPause();
}

void BakaEngine::BakaVideoSize(QStringList &args)
{
    if (args.empty())
        FitWindow();
    else {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            FitWindow(arg.toInt());
        else
            InvalidParameter(args.join(' '));
    }
}

void BakaEngine::FitWindow(int percent, bool msg)
{
    if (window->isFullScreen() || window->isMaximized())
        return;

    const Mpv::VideoParams &vG = mpv->getFileInfo().video_params; // video geometry
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

    int gcd = Util::GCD(vG.width, vG.height);
    if (gcd)
        Util::SetAspectRatio(window, vG.width / gcd, vG.height / gcd);

    if(msg)
        mpv->ShowText(tr("Window Size: %0").arg(percent == 0 ? tr("Fit to Screen") : (QString::number(percent)+"%")));
}

void BakaEngine::BakaDeinterlace(QStringList &args)
{
    if (args.empty())
        mpv->Deinterlace(window->ui->action_Deinterlace->isChecked());
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaInterpolate(QStringList &args)
{
    if (args.empty())
        mpv->Interpolate(window->ui->action_Motion_Interpolation->isChecked());
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaMute(QStringList &args)
{
    if (args.empty())
        mpv->Mute(!mpv->getMute());
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaVolume(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->Volume(mpv->getVolume()+arg.toInt(), true);
            else
                mpv->Volume(arg.toInt(), true);
        } else
            InvalidParameter(args.join(' '));
    } else
        RequiresParameters("volume");
}

void BakaEngine::BakaAudioDelay(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->AudioDelay(mpv->getAudioDelay()+arg.toDouble());
            else
                mpv->AudioDelay(arg.toDouble());
            mpv->ShowText(tr("Audio Delay: %0").arg(QString::number(mpv->getAudioDelay(), 'f', 1)));
        } else
            InvalidParameter(args.join(' '));
    } else
        RequiresParameters("audio_delay");
}

void BakaEngine::BakaSubtitleDelay(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->SubtitleDelay(mpv->getSubtitleDelay()+arg.toDouble());
            else
                mpv->SubtitleDelay(arg.toDouble());
            mpv->ShowText(tr("Subtitle Delay: %0").arg(QString::number(mpv->getSubtitleDelay(), 'f', 1)));
        } else
            InvalidParameter(args.join(' '));
    } else
        RequiresParameters("subtitle_delay");
}

void BakaEngine::BakaSubtitleFont(QStringList &args)
{
    if (args.empty()) {
        bool ok = false;
        QFont font = QFontDialog::getFont(&ok, mpv->getSubtitleFont(), window, tr("Set Subtitle Font"),
                                          QFontDialog::DontUseNativeDialog);
        if (ok)
            mpv->SubtitleFont(font);
    } else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaSubtitleStyle(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg == "color") {
                QColor color = QColorDialog::getColor(mpv->getSubtitleColor(), window, tr("Set Subtitle Color"),
                                                      QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
                if (color.isValid())
                    mpv->SubtitleColor(color);
            } else if (arg == "back-color") {
                QColor color = QColorDialog::getColor(mpv->getSubtitleBackColor(), window, tr("Set Subtitle Background Color"),
                                                      QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
                if (color.isValid())
                    mpv->SubtitleBackColor(color);
            } else if (arg == "blur") {
                QString input = InputDialog::getInput(tr("Input Blur Factor (0-20.0):"), tr("Set Blur Factor"), [=] (QString input) {
                    double v = input.toDouble();
                    return v >= 0 && v <= 20.0;
                }, window);
                if (!input.isEmpty()) {
                    double factor = input.toDouble();
                    mpv->SubtitleBlur(factor);
                    window->ui->action_Subtitle_Blur->setChecked(factor);
                }
            } else if (arg == "shadow-offset") {
                QString input = InputDialog::getInput(tr("Input Offset Value (0-20):"), tr("Set Shadow Offset"), [=] (QString input) {
                    int v = input.toInt();
                    return v >= 0 && v <= 20;
                }, window);
                if (!input.isEmpty()) {
                    int offset = input.toInt();
                    mpv->SubtitleShadowOffset(offset);
                    window->ui->action_Subtitle_Shadow_Offset->setChecked(offset);
                }
            } else if (arg == "shadow-color") {
                QColor color = QColorDialog::getColor(mpv->getSubtitleShadowColor(), window, tr("Set Shadow Color"),
                                                      QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
                if (color.isValid())
                    mpv->SubtitleShadowColor(color);
            } else
                InvalidParameter(arg);
        } else
            InvalidParameter(args.join(' '));
    } else
        RequiresParameters("subtitle_style");
}

void BakaEngine::BakaSpeed(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            if (arg.startsWith('+') || arg.startsWith('-'))
                mpv->Speed(mpv->getSpeed()+arg.toDouble());
            else
                mpv->Speed(arg.toDouble());
            mpv->ShowText(tr("Speed: %0x").arg(QString::number(mpv->getSpeed(), 'f', 2)));
        } else
            InvalidParameter(args.join(' '));
    } else
        RequiresParameters("speed");
}

void BakaEngine::BakaFullScreen(QStringList &args)
{
    if (args.empty()) {
        window->FullScreen(!window->isFullScreen());
        if (window->isFullScreen())
            mpv->ShowText(tr("Press ESC or double-click to leave full screen"));
    } else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaBoss(QStringList &args)
{
    if (args.empty()) {
        mpv->Pause();
        window->setWindowState(window->windowState() | Qt::WindowMinimized); // minimize window
    } else
        InvalidParameter(args.join(' '));
}

void BakaEngine::BakaHelp(QStringList &args)
{
    if (args.empty()) {
        Print(tr("usage: baka <command> [...]"));
        Print(tr("commands:"));
        int len, max_len = 22;
        for (auto command = BakaCommandMap.begin(); command != BakaCommandMap.end(); ++command) {
            QString str = QString("  %0 %1").arg(command.key(), command->second[0]);
            len = str.length();
            while (len++ <= max_len)
                str += ' ';
            str += command->second[1];
            Print(str);
        }
    } else {
        QString arg = args.front();
        args.pop_front();
        if (args.empty()) {
            auto command = BakaCommandMap.find(arg);
            if (command != BakaCommandMap.end()) { //found
                Print(tr("usage: %0 %1").arg(arg, command->second[0]));
                Print(tr("description:"));
                Print(QString("  %0").arg(command->second[1]));
                if (command->second.length() > 2 && command->second[2] != QString()) {
                    Print(tr("advanced:"));
                    Print(QString("  %0").arg(command->second[2]));
                }
            } else
                InvalidParameter(arg);
        } else
            InvalidParameter(args.join(' '));
    }
}

void BakaEngine::BakaAbout(QStringList &args)
{
    About(args.join(' '));
}

void BakaEngine::BakaMsgLevel(QStringList &args)
{
    if (!args.empty()) {
        QString arg = args.front();
        args.pop_front();
        if (args.empty())
            mpv->MsgLevel(arg);
        else
            InvalidParameter(args.join(' '));
    } else
        RequiresParameters("msg_level");
}

void BakaEngine::About(QString what)
{
    if (what == QString())
        AboutDialog::about(APP_VERSION, window);
    else if (what == "qt")
        qApp->aboutQt();
    else
        InvalidParameter(what);
}


void BakaEngine::BakaQuit(QStringList &args)
{
    if (args.empty())
        Quit();
    else
        InvalidParameter(args.join(' '));
}

void BakaEngine::Quit()
{
    qApp->quit();
}
