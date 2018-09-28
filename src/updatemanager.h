#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <QString>

class BakaEngine;

class UpdateManager : public QObject {
    Q_OBJECT
public:
    explicit UpdateManager(QObject *parent = 0);
    ~UpdateManager();

    const QMap<QString, QString> &getInfo() { return info; }

public slots:
    bool checkForUpdates();

#if defined(Q_OS_WIN)
    bool downloadUpdate(const QString &url);
    void applyUpdate(const QString &file);
#endif

signals:
    void progressSignal(int percent);
    void messageSignal(QString msg);

private:
    BakaEngine *baka;
    QMap<QString, QString> info;
};

#endif // UPDATEMANAGER_H
