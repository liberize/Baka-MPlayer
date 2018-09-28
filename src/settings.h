#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

class BakaEngine;

class Settings : public QObject {
    Q_OBJECT
public:
    explicit Settings(QString file, QObject *parent = 0);
    ~Settings();

public slots:
    void load();
    void save();

    QJsonObject getRoot();
    void setRoot(QJsonObject);

protected:
    void loadIni();
    int parseLine(QString line);
    QString fixKeyOnLoad(QString key);
    QStringList splitQStringList(QString list);

private:
    BakaEngine *baka;
    QJsonDocument document;
    QString file;
};

#endif // SETTINGS_H
