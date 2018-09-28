#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "plugin.h"
#include "worker.h"

#include <QStringList>
#include <QMap>
#include <QList>
#include <QSet>
#include <QThread>
#include <QQueue>

class BakaEngine;

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

    Worker *newWorker(Worker::Priority priority = Worker::Normal);
    void runNextWorker();
    void deleteWorker(Worker *worker);
    void clearWorkers(Worker::Priority maxPriority);

signals:
    void pluginsLoaded(const QMap<QString, Plugin*> &plugins);
    void error(QString msg);

private:
    BakaEngine *baka = nullptr;
    py::module module;
    QSet<QString> disableList;
    QMap<QString, Plugin*> plugins;
    QQueue<Worker*> workerQueue;
    bool busy = false;
};

#endif // PLUGINMANAGER_H
