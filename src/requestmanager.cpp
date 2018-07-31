#include "requestmanager.h"
#include "fetchrequest.h"

#include <QNetworkRequest>
#include <QNetworkReply>

#include "bakaengine.h"
#include "util.h"

RequestManager::RequestManager(QObject *parent) :
    QObject(parent),
    baka(static_cast<BakaEngine*>(parent)),
    manager(new QNetworkAccessManager(this))
{
}

RequestManager::~RequestManager()
{
    delete manager;
}

FetchRequest *RequestManager::newRequest(QString url)
{
    return new FetchRequest(url, baka, this);
}

QNetworkReply *RequestManager::getReply(QUrl url)
{
    return manager->get(QNetworkRequest(url));
}
