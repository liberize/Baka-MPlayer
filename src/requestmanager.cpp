#include "requestmanager.h"
#include "request.h"

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

Request *RequestManager::newRequest(QString url, const QByteArray &postData, const QMap<QByteArray, QByteArray> &headers)
{
    Request *req = new Request(url, this);
    req->setHeaders(headers);
    req->setPostData(postData);
    return req;
}

QNetworkReply *RequestManager::send(Request *req)
{
    QNetworkRequest request(req->getUrl());
    for (auto it = req->getHeaders().begin(); it != req->getHeaders().end(); ++it)
        request.setRawHeader(it.key(), *it);

    if (!req->getPostData().isEmpty())
        return manager->post(request, req->getPostData());
    return manager->get(request);
}

QString RequestManager::getSaveDir()
{
    return baka->tempDir->path();
}
