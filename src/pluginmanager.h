#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "plugintypes.h"

#include <QStringList>
#include <QMap>
#include <QList>
#include <QSet>
#include <QThread>

class BakaEngine;

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = 0);
    ~PluginManager();

    QSet<QString> &GetDisableList() { return disableList; }
    void LoadPlugins();
    QList<Pi::Plugin> GetAllPlugins();
    QList<Pi::Plugin> GetSubtitlePlugins();
    QList<Pi::Plugin> GetMediaPlugins();
    bool IsSubtitlePlugin(QString name);
    bool IsMediaPlugin(QString name);
    void EnablePlugin(QString name, bool enable);
    bool UpdatePluginConfig(QString name, const QList<Pi::ConfigItem> &config);
    bool SearchSubtitle(QString name, QString word, int count);
    bool DownloadSubtitle(QString name, const Pi::SubtitleEntry &entry);
    bool FetchMedia(QString name, int start, int count);
    bool SearchMedia(QString name, QString word, int count);
    bool DownloadMedia(QString name, const Pi::MediaEntry &entry);

signals:
    void PluginStateChanged(QString name, bool enable);
    void SearchSubtitleFinished(QString name, QList<Pi::SubtitleEntry> result);
    void DownloadSubtitleFinished(QString name, Pi::SubtitleEntry entry);
    void FetchMediaFinished(QString name, QList<Pi::MediaEntry> result);
    void SearchMediaFinished(QString name, QList<Pi::MediaEntry> result);
    void DownloadMediaFinished(QString name, Pi::MediaEntry entry);

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
    BakaEngine *baka = nullptr;
    py::module wrapper;
    bool pluginsLoaded = false;
    QSet<QString> disableList;
    QThread *workerThread = nullptr;
};

#endif // PLUGINMANAGER_H
