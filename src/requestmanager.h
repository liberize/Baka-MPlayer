#ifndef REQUESTMANAGER_H
#define REQUESTMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QMap>
#include <QByteArray>

class FetchRequest;
class BakaEngine;

class RequestManager : public QObject {
    Q_OBJECT
public:
    explicit RequestManager(QObject *parent = 0);
    ~RequestManager();

    FetchRequest *newRequest(QString url, const QByteArray &postData = QByteArray(),
                             const QMap<QByteArray, QByteArray> &headers = QMap<QByteArray, QByteArray>());
    QNetworkReply *sendRequest(FetchRequest *req);

private:
    BakaEngine *baka = nullptr;
    QNetworkAccessManager *manager = nullptr;
};

#endif // REQUESTMANAGER_H
