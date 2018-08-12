#include "updatemanager.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QList>
#include <QByteArray>
#include <QUrl>
#include <QStringList>
#include <QFile>
#include <QProcess>
#include <QtGlobal>

#include "request.h"
#include "requestmanager.h"

#if defined(Q_OS_WIN)
#include <QDir>
#include <zip.h>
#endif

#include "bakaengine.h"
#include "util.h"

UpdateManager::UpdateManager(QObject *parent) :
    QObject(parent),
    baka(static_cast<BakaEngine*>(parent))
{
}

UpdateManager::~UpdateManager()
{
}

bool UpdateManager::CheckForUpdates()
{
    emit messageSignal(tr("Checking for updates..."));

    Request *req = baka->requestManager->newRequest(Util::VersionFileUrl());
    connect(req, &Request::progress, [=] (double percent) {
        emit progressSignal((int)(50.0 * percent));
    });
    connect(req, &Request::error, [=] (QString msg) {
        emit messageSignal(msg);
        req->deleteLater();
    });
    connect(req, &Request::message, [=] (QString msg) {
        emit messageSignal(msg);
    });
    connect(req, &Request::fetched, [=] (QByteArray bytes) {
        QList<QByteArray> lines = bytes.split('\n');
        QList<QByteArray> pair;
        QString lastPair;
        // go through the next 50% incrementally during parsing
        double amnt = 50.0 / lines.length();
        double cur = 50 + amnt;
        for (auto line : lines) {
            if ((pair = line.split('=')).size() != 2)
                info[lastPair].append(line);
            else
                info[(lastPair = pair[0])] = QString(pair[1]);
            emit progressSignal((int)(cur += amnt));
        }
        req->deleteLater();
    });
    return req->fetch();
}

#if defined(Q_OS_WIN)
bool UpdateManager::DownloadUpdate(const QString &url)
{
    emit messageSignal(tr("Downloading update..."));

    Request *req = baka->requestManager->newRequest(url);
    connect(req, &Request::progress, [=] (double percent) {
        emit progressSignal((int)(99.0 * percent));
    });
    connect(req, &Request::error, [=] (QString msg) {
        emit messageSignal(msg);
        req->deleteLater();
    });
    connect(req, &Request::message, [=] (QString msg) {
        emit messageSignal(msg);
    });
    connect(req, &Request::saved, [=] (QString filePath) {
        ApplyUpdate(filePath);
        req->deleteLater();
    });

    QString filePath = QDir::toNativeSeparators(QString("%0/Baka-MPlayer.zip").arg(QCoreApplication::applicationDirPath()));
    return req->fetch(true, filePath);
}

void UpdateManager::ApplyUpdate(const QString &file)
{
    emit messageSignal(tr("Extracting..."));
    // create a temporary directory for baka
    QString path = QDir::toNativeSeparators(QString("%0/.tmp/").arg(QCoreApplication::applicationDirPath()));
    QString exe = QDir::toNativeSeparators(QString("%0/Baka MPlayer.exe").arg(QCoreApplication::applicationDirPath()));
    QString bat = QDir::toNativeSeparators(QString("%0/updater.bat").arg(QCoreApplication::applicationDirPath()));
    QDir dir;
    dir.mkpath(path);
    int err;
    struct zip *z = zip_open(file.toUtf8(), 0, &err);
    int n = zip_get_num_entries(z, 0);
    for (int64_t i = 0; i < n; ++i) {
        // get file stats
        struct zip_stat s;
        zip_stat_index(z, i, 0, &s);
        // extract file
        char *buf = new char[s.size]; // allocate buffer
        // extract file to buffer
        zip_file *zf = zip_fopen_index(z, i, 0);
        zip_fread(zf, buf, s.size);
        zip_fclose(zf);
        // write new file
        QFile f(path + s.name);
        f.open(QFile::WriteOnly | QFile::Truncate);
        f.write(buf, s.size);
        f.close();
    }
    zip_close(z);
    // write updater batch script
    emit messageSignal(tr("Creating updater script..."));
    QFile f(bat);
    if (!f.open(QFile::WriteOnly | QFile::Truncate)) {
        emit messageSignal(tr("Could not open file for writing..."));
        return;
    }
    f.write(
        QString(
            "@echo off\r\n"
            "echo %0\r\n"                                               // status message
            "ping 127.0.0.1 -n 1 -w 1000 > NUL\r\n"                     // wait while baka closes
            "cd \"%1\"\r\n"                                             // go to extracted directory
            "for %%i in (*) do move /Y \"%%i\" ..\r\n"                  // move all files up
            "for /d %%i in (*) do move /Y \"%%i\" ..\r\n"               // move all directories up
            "cd ..\r\n"                                                 // go up
            "rmdir /Q /S \"%2\"\r\n"                                    // remove the tmp directory
            "del /Q \"%3\"\r\n"                                         // delete the zip file
            "start /b \"\" \"%4\"\r\n"                                  // start the new upv
            "start /b \"\" cmd /c del \"%~f0\"&exit /b\"\"\r\n").arg(   // remove the script itself and exit
            tr("Updating..."),
            path,
            path,
            file,
            exe).toUtf8());
    f.close();

    QProcess::startDetached(bat);
    emit messageSignal(tr("Done. Restarting..."));
    baka->Quit();
}
#endif
