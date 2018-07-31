#ifndef CONTENTFETCHER_H
#define CONTENTFETCHER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <QString>
#include <QTemporaryDir>

class BakaEngine;

class ContentFetcher : public QObject {
    Q_OBJECT
public:
    explicit ContentFetcher(QObject *parent = 0);
    ~ContentFetcher();

    bool fetch(QString url, bool saveToFile = false, QString savePath = QString(), bool overwrite = false);

signals:
    void progress(double percent);
    void fetched(QByteArray data);
    void fetched(QString filePath);
    void message(bool error, QString msg);

private:
    void doFetch(QUrl url);
    bool open(QString path, bool overwrite);
    void write(const QByteArray &bytes);
    void flush();
    void rewind();
    void close(bool success);

private:
    QNetworkAccessManager *manager = nullptr;
    QTemporaryDir *tempDir = nullptr;
    bool busy = false;
    bool saveToFile = false;
    QFile *file = nullptr;
    QByteArray *data = nullptr;
};

#endif // CONTENTFETCHER_H
