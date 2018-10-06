#include "mtspmessagehandler.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QDebug>

namespace {
const int DEFAULT_PORT = 63452;
const int MAX_PORT = DEFAULT_PORT + 100;
}

MtspMessageHandler::MtspMessageHandler(QObject *parent)
    : QObject(parent)
{
    udpSocket = new QUdpSocket(this);

    int port = DEFAULT_PORT;
    while (!udpSocket->bind(QHostAddress::LocalHost, port) && port < MAX_PORT)
        port++;
    if (port == MAX_PORT)
        qDebug() << "failed to bind port";

    connect(udpSocket, &QUdpSocket::readyRead, [=] {
        QByteArray datagram;
        while (udpSocket->hasPendingDatagrams()) {
            datagram.resize(udpSocket->pendingDatagramSize());
            udpSocket->readDatagram(datagram.data(), datagram.size());
            processMessage(datagram);
        }
    });
}

MtspMessageHandler::~MtspMessageHandler()
{
    delete udpSocket;
}

void MtspMessageHandler::processMessage(QByteArray &message)
{
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(message, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "parse json failed:" << message;
        return;
    }
    QJsonObject root = document.object();
    QString type = QJsonValueRef(root["type"]).toString("");
    if (type == "buffer-ranges") {
        QList<QPair<double, double>> ranges;
        for (auto range : root["data"].toArray()) {
            QJsonObject r = range.toObject();
            double from = QJsonValueRef(r["from"]).toDouble();
            double to = QJsonValueRef(r["to"]).toDouble();
            ranges.append(QPair<double, double>(from, to));
        }
        emit bufferRangesUpdated(ranges);
    }
}
