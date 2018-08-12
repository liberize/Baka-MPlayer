#ifndef MEDIAPROVIDER_H
#define MEDIAPROVIDER_H

#include "plugin.h"


class MediaProvider : public Plugin {
    Q_OBJECT
public:
    using Plugin::Plugin;

    bool fetch(int start, int count);
    bool search(QString word, int count);
    bool download(const MediaEntry &entry);

signals:
    void fetchFinished(const QList<MediaEntry> &result);
    void searchFinished(const QList<MediaEntry> &result);
    void downloadFinished(const MediaEntry &entry);
};

#endif // MEDIAPROVIDER_H
