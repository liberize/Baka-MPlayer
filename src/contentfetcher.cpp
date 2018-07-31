#include "contentfetcher.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QList>
#include <QByteArray>
#include <QUrl>
#include <QStringList>
#include <QFile>
#include <QProcess>
#include <QTemporaryFile>

#include "util.h"

ContentFetcher::ContentFetcher(QObject *parent) :
    QObject(parent),
    manager(new QNetworkAccessManager(this))
{
}

ContentFetcher::~ContentFetcher()
{
    delete manager;
    if (tempDir)
        delete tempDir;
}

bool ContentFetcher::open(QString path, bool overwrite)
{
    if (!saveToFile) {
        data = new QByteArray;
        return true;
    }

    if (path.isEmpty()) {
        if (!tempDir)
            tempDir = new QTemporaryDir;
        QTemporaryFile *tempFile = new QTemporaryFile(tempDir->path());
        tempFile->setAutoRemove(false);
        file = tempFile;
    } else {
        QFileInfo info(path);
        if (info.isDir()) {
            // for simplicity, use url filename instead of parsing Content-Disposition header
            path = Util::Path(savePath, QUrl(url).fileName());
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

void ContentFetcher::write(const QByteArray &bytes)
{
    if (saveToFile) {
        if (file->write(bytes) == -1)
            emit message(true, tr("write error"));
    } else
        data->append(bytes);
}

void ContentFetcher::flush()
{
    if (saveToFile)
        file->flush();
}

void ContentFetcher::rewind()
{
    if (saveToFile)
        file->seek(0);
    else
        data->clear();
}

void ContentFetcher::close(bool success)
{
    if (saveToFile) {
        if (success)
            emit fetched(file->fileName());
        file->close();
        delete file;
        file = nullptr;
    } else {
        if (success)
            emit fetched(*data);
        delete data;
        data = nullptr;
    }
}

bool ContentFetcher::fetch(QString url, bool saveToFile, QString savePath, bool overwrite)
{
    if (busy)
        return false;

    busy = true;
    if (!open(savePath, overwrite)) {
        emit message(true, tr("fopen error\n"));
        busy = false;
        return false;
    }

    doFetch(QUrl(url));
    return true;
}

void ContentFetcher::doFetch(QUrl url)
{
    busy = true;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 received, qint64 total) {
        emit progress((double)received / total);
    });

    connect(reply, &QNetworkReply::readyRead, [=] {
        if (reply->error())
            emit message(true, reply->errorString());
        else
            write(reply->read(reply->bytesAvailable()));
    });

    connect(reply, &QNetworkReply::finished, [=] {
        busy = false;
        if (reply->error()) {
            emit message(true, reply->errorString());
            close(false);
        } else {
            flush();
            QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (redirect.isEmpty()) {
                emit message(false, tr("Download complete"));
                emit progress(1);
                close(true);
            } else {
                emit message(false, tr("Redirected..."));
                rewind();
                doFetch(std::move(redirect));
            }
        }
        reply->deleteLater();
    });

    emit progress(0);
}
