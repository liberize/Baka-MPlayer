#ifndef RECENT_H
#define RECENT_H

#include <QString>

struct Recent {
    Recent(QString s = QString(), QString t = QString(), double p = 0):
        path(s), title(t), time(p) {}

    operator QString() const { return path; }
    bool operator==(const Recent &recent) const { return (path == recent.path); }

    QString path;
    QString title;
    double time;
};

#endif // RECENT_H
