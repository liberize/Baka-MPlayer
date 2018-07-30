#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "plugintypes.h"

#include <QStringList>
#include <QMap>
#include <QList>
#include <QSet>
#include <QThread>


class PluginManager : public QObject {
    friend class BakaEngine;
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = 0);
    ~PluginManager();

    QSet<QString> &GetDisableList() { return disableList; }
    void LoadPlugins();
    QList<Pi::Plugin> GetAllPlugins();
    QList<Pi::Plugin> GetSubtitlePlugins();
    QList<Pi::Plugin> GetMediaPlugins();
    void EnablePlugin(QString name, bool enable);
    void UpdatePluginConfig(QString name, const QList<Pi::ConfigItem> &config);
    bool SearchSubtitle(QString name, QString word, int count = 10);
    bool FetchMedia(QString name, int count = 50);
    bool SearchMedia(QString name, QString word, int count = 50);

signals:
    void SearchSubtitleFinished(QString name, QList<Pi::SubtitleEntry> result);
    void FetchMediaFinished(QString name, QList<Pi::MediaEntry> result);
    void SearchMediaFinished(QString name, QList<Pi::MediaEntry> result);

private:
    template <typename T>
    T SafeRun(std::function<T()> func) {
        try {
            return func();
        } catch (std::runtime_error e) {
            qDebug() << e.what();
        }
        return T();
    }
    void RunWorker(std::function<void()> func);

private:
    py::module wrapper;
    bool pluginsLoaded = false;
    QSet<QString> disableList;
    QThread *workerThread = nullptr;
};

#endif // PLUGINMANAGER_H
