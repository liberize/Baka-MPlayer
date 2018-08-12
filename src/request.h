#ifndef REQUEST_H
#define REQUEST_H

#include <QObject>
#include <QFile>
#include <QUrl>
#include <QNetworkReply>
#include <QMap>
#include <QByteArray>

class RequestManager;

class Request : public QObject {
    Q_OBJECT
public:
    explicit Request(QString url, QObject *parent = nullptr);
    ~Request();

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
    RequestManager *manager = nullptr;
    QNetworkReply *currentReply = nullptr;
};

#endif // REQUEST_H
