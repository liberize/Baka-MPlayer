#include "fetchrequest.h"
#include "requestmanager.h"
#include "bakaengine.h"
#include "util.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>
#include <QUrl>
#include <QTemporaryFile>


FetchRequest::FetchRequest(QString url, BakaEngine *baka, QObject *parent)
    : QObject(parent), url(url), baka(baka)
{
}

FetchRequest::~FetchRequest()
{
    abort();
    closeFile();
}

bool FetchRequest::fetch(bool saveToFile, QString savePath, bool overwrite)
{
    if (saveToFile) {
        if (!openFile(savePath, overwrite)) {
            emit error(tr("fopen error\n"));
            return false;
        }
    }
    doFetch();
    return true;
}

void FetchRequest::abort()
{
    if (currentReply) {
        currentReply->abort();
        currentReply = nullptr;
    }
}

void FetchRequest::doFetch()
{
    QNetworkReply *reply = baka->requestManager->sendRequest(this);
    currentReply = reply;

    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 received, qint64 total) {
        emit progress((double)received / total);
    });

    connect(reply, &QNetworkReply::readyRead, [=] {
        if (reply->error())
            emit error(reply->errorString());
        else if (file)
            if (file->write(reply->read(reply->bytesAvailable())) == -1)
                emit error(tr("write error"));
    });

    connect(reply, &QNetworkReply::finished, [=] {
        currentReply = nullptr;
        if (reply->error()) {
            emit error(reply->errorString());
            closeFile();
        } else {
            if (file)
                file->flush();
            QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (redirect.isEmpty()) {
                emit message(tr("Download complete"));
                emit progress(1);
                if (file) {
                    QString path = file->fileName();
                    closeFile();
                    emit saved(path);
                } else
                    emit fetched(reply->readAll());
            } else {
                emit message(tr("Redirected..."));
                if (file)
                    file->seek(0);
                url = redirect;
                doFetch();
            }
        }
        reply->deleteLater();
    });

    emit progress(0);
}

bool FetchRequest::openFile(QString path, bool overwrite)
{
    if (path.isEmpty()) {
        QTemporaryFile *tempFile = new QTemporaryFile(baka->tempDir->path());
        tempFile->setAutoRemove(false);
        file = tempFile;
    } else {
        QFileInfo info(path);
        if (info.isDir()) {
            // for simplicity, use url filename instead of parsing Content-Disposition header
            path = Util::Path(path, url.fileName());
            info.setFile(path);
        }
        if (info.exists() && !overwrite) {
            QString suffix = info.completeSuffix();
            QString pathTemplate = suffix.isEmpty() ? path + "_%0" : info.dir().filePath(info.baseName() + "_%0." + suffix);
            for (int i = 1; QFile::exists(path = pathTemplate.arg(QString::number(i))); i++);
        }
        file = new QFile(path);
    }
    if (!file->open(QFile::WriteOnly | QFile::Truncate)) {
        delete file;
        file = nullptr;
    }
    return file;
}

void FetchRequest::closeFile()
{
    if (file) {
        file->close();
        delete file;
        file = nullptr;
    }
}
