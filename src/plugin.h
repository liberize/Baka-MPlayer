#ifndef PLUGIN_H
#define PLUGIN_H

#include "plugintypes.h"

#include <QObject>

class PluginManager;

class Plugin : public QObject {
    Q_OBJECT
public:
    explicit Plugin(const py::object &obj, QObject *parent = nullptr);
    ~Plugin();

    QString getName() const { return name; }
    const QIcon &getIcon() const { return icon; }
    QString getDescription() const { return description; }
    const QVector<ConfigItem> &getConfig() const { return config; }
    QString getPath() const { return path; }
    bool isEnabled() const { return enabled; }

    bool isSubtitleProvider();
    bool isMediaProvider();
    void setEnabled(bool enable);
    void updateConfig(const QVector<ConfigItem> &conf);

signals:
    void enableStateChanged(bool enable);
    void configUpdated(const QVector<ConfigItem> &config);

protected:
    QString name;
    QIcon icon;
    QString description;
    QVector<ConfigItem> config;
    QString path;
    bool enabled = true;

    py::object plugin;
    PluginManager *manager = nullptr;
};

#endif // PLUGIN_H
