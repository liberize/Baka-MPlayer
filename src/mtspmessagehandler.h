#ifndef MTSPMESSAGEHANDLER_H
#define MTSPMESSAGEHANDLER_H

#include <QObject>
#include <QUdpSocket>

class MtspMessageHandler : public QObject
{
    Q_OBJECT
public:
    explicit MtspMessageHandler(QObject *parent = nullptr);
    ~MtspMessageHandler();

    int getPort() { return udpSocket->localPort(); }

signals:
    void bufferRangesUpdated(QList<QPair<double, double>> &ranges);

private:
    void processMessage(QByteArray &message);

private:
    QUdpSocket *udpSocket = nullptr;
};

#endif // MTSPMESSAGEHANDLER_H
