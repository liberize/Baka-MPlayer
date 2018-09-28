#ifndef MEDIAPROVIDER_H
#define MEDIAPROVIDER_H

#include "plugin.h"


class MediaProvider : public Plugin {
    Q_OBJECT
public:
    using Plugin::Plugin;

    void fetch(int start, int count = 20);
    void search(QString word, int count = 100);
    void download(const MediaEntry &entry, QString what, const QPersistentModelIndex &index);

    void fetchNext(int count = 20);

signals:
    void fetchFinished(const QList<MediaEntry> &result);
    void searchFinished(const QList<MediaEntry> &result);
    void downloadFinished(const MediaEntry &entry, QString what, const QPersistentModelIndex &index);
    void error(QString msg);

private:
    int nextStart = 0;
    bool fetching = false;
};

#endif // MEDIAPROVIDER_H
