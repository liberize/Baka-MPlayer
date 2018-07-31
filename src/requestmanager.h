#ifndef REQUESTMANAGER_H
#define REQUESTMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

class FetchRequest;
class BakaEngine;

class RequestManager : public QObject {
    Q_OBJECT
public:
    explicit RequestManager(QObject *parent = 0);
    ~RequestManager();

    FetchRequest *newRequest(QString url);
    QNetworkReply *getReply(QUrl url);

private:
    BakaEngine *baka = nullptr;
    QNetworkAccessManager *manager = nullptr;
};

#endif // REQUESTMANAGER_H
