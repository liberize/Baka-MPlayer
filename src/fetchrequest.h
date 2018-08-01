#ifndef FETCHREQUEST_H
#define FETCHREQUEST_H

#include <QObject>
#include <QFile>
#include <QUrl>
#include <QNetworkReply>
#include <QMap>
#include <QByteArray>

class BakaEngine;

class FetchRequest : public QObject {
    Q_OBJECT
public:
    explicit FetchRequest(QString url, BakaEngine *baka, QObject *parent = nullptr);
    ~FetchRequest();

    const QUrl &getUrl() const { return url; }
    QByteArray &getPostData() { return postData; }
    QMap<QByteArray, QByteArray> &getHeaders() { return headers; }
    void setPostData(const QByteArray &d) { postData = d; }
    void setHeaders(const QMap<QByteArray, QByteArray> &h) { headers = h; }

    bool fetch(bool saveToFile = false, QString savePath = QString(), bool overwrite = false);
    void abort();

signals:
    void progress(double percent);
    void fetched(QByteArray data);
    void saved(QString filePath);
    void message(QString msg);
    void error(QString msg);

private:
    void doFetch();
    bool openFile(QString path, bool overwrite);
    void closeFile();

private:
    QUrl url;
    QByteArray postData;
    QMap<QByteArray, QByteArray> headers;
    QFile *file = nullptr;
    BakaEngine *baka = nullptr;
    QNetworkReply *currentReply = nullptr;
};

#endif // FETCHREQUEST_H
