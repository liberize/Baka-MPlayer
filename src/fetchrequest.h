#ifndef FETCHREQUEST_H
#define FETCHREQUEST_H

#include <QObject>
#include <QFile>
#include <QUrl>
#include <QNetworkReply>

class BakaEngine;

class FetchRequest : public QObject  {
    Q_OBJECT
public:
    explicit FetchRequest(QString url, BakaEngine *baka, QObject *parent = nullptr);
    ~FetchRequest();

    bool fetch(bool saveToFile = false, QString savePath = QString(), bool overwrite = false);
    void abort();

signals:
    void progress(double percent);
    void fetched(QByteArray data);
    void saved(QString filePath);
    void message(QString msg);
    void error(QString msg);

private:
    void doFetch(QUrl currentUrl);
    bool openFile(QString path, bool overwrite);
    void closeFile();

private:
    QUrl url;
    QFile *file = nullptr;
    BakaEngine *baka = nullptr;
    QNetworkReply *currentReply = nullptr;
};

#endif // FETCHREQUEST_H
