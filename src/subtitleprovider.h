#ifndef SUBTITLEPROVIDER_H
#define SUBTITLEPROVIDER_H

#include "plugin.h"

class SubtitleProvider : public Plugin {
    Q_OBJECT
public:
    using Plugin::Plugin;

    void search(QString word, int count = 10);
    void download(const SubtitleEntry &entry);

signals:
    void searchFinished(const QList<SubtitleEntry> &result);
    void downloadFinished(const SubtitleEntry &entry);
};

#endif // SUBTITLEPROVIDER_H
