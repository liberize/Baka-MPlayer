#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "plugin.h"

#include <QStringList>
#include <QMap>
#include <QList>
#include <QSet>
#include <QThread>

class BakaEngine;
class Worker;

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = 0);
    ~PluginManager();

    py::module &getModule() { return module; }
    QSet<QString> &getDisableList() { return disableList; }
    const QMap<QString, Plugin*> &getPlugins() const { return plugins; }

    void loadPlugins();
    Plugin *findPlugin(QString name);

    Worker *newWorker();
    void runNextWorker();
    void deleteWorker(Worker *worker);

signals:
    void pluginsLoaded(const QMap<QString, Plugin*> &plugins);

private:
    BakaEngine *baka = nullptr;
    py::module module;
    QSet<QString> disableList;
    QMap<QString, Plugin*> plugins;
    QList<Worker*> workerQueue;
    bool busy = false;
};

#endif // PLUGINMANAGER_H
