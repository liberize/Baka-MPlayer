#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "plugintypes.h"

#include <QStringList>
#include <QMap>
#include <QList>


class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = 0);
    ~PluginManager();

    void LoadPlugins(const QList<QString> &disableList);
    QList<Pi::Plugin> GetAllPlugins();
    QList<Pi::Plugin> GetSubtitlePlugins();
    QList<Pi::Plugin> GetMediaPlugins();
    void EnablePlugin(QString name, bool enable);
    void UpdatePluginConfig(QString name, const QList<Pi::ConfigItem> &config);
    QList<Pi::SubtitleEntry> SearchSubtitle(QString name, QString word, int count);
    QList<Pi::MediaEntry> FetchMedia(QString name, int count);
    QList<Pi::MediaEntry> SearchMedia(QString name, QString word, int count);

private:
    py::module wrapper;
};

#endif // PLUGINMANAGER_H
