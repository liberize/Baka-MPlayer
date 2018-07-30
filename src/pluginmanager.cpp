#include "pluginmanager.h"
#include "util.h"

#include <QDebug>

using namespace Pi;

#define SAFE_RUN()

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
    SafeRun<void>([=] {
        py::initialize_interpreter();

        py::module sys = py::module::import("sys");
        const char *scriptsPath = Util::ScriptsPath().toUtf8().constData();
        sys.attr("path").cast<py::list>().append(scriptsPath);

        wrapper = py::module::import("upvwrapper");
    });
}

PluginManager::~PluginManager()
{
    SafeRun<void>([=] {
        wrapper.release();
        py::finalize_interpreter();
    });
}

void PluginManager::RunWorker(std::function<void()> func)
{
    workerThread = new QThread;
    connect(workerThread, &QThread::started, [=] {
        SafeRun<void>(func);
        workerThread->exit();
    });
    connect(workerThread, &QThread::finished, this, [=] {
        delete workerThread;
        workerThread = nullptr;
    }, Qt::QueuedConnection);
    workerThread->start();
}

void PluginManager::LoadPlugins()
{
    SafeRun<void>([=] {
        wrapper.attr("load_plugins")(Util::PluginsPaths(), disableList.toList());
        pluginsLoaded = true;
    });
}

QList<Pi::Plugin> PluginManager::GetAllPlugins()
{
    if (!pluginsLoaded)
        return QList<Plugin>();

    return SafeRun<QList<Plugin>>([=] {
        return wrapper.attr("get_all_plugins")().cast<QList<Plugin>>();
    });
}

QList<Plugin> PluginManager::GetSubtitlePlugins()
{
    if (!pluginsLoaded)
        return QList<Plugin>();

    return SafeRun<QList<Plugin>>([=] {
        return wrapper.attr("get_subtitle_plugins")().cast<QList<Plugin>>();
    });
}

QList<Plugin> PluginManager::GetMediaPlugins()
{
    if (!pluginsLoaded)
        return QList<Plugin>();

    return SafeRun<QList<Plugin>>([=] {
        return wrapper.attr("get_media_plugins")().cast<QList<Plugin>>();
    });
}

bool PluginManager::IsSubtitlePlugin(QString name)
{
    if (!pluginsLoaded)
        return false;

    return SafeRun<bool>([=] {
        return wrapper.attr("is_subtitle_plugin")().cast<bool>();
    });
}

bool PluginManager::IsMediaPlugin(QString name)
{
    if (!pluginsLoaded)
        return false;

    return SafeRun<bool>([=] {
        return wrapper.attr("is_media_plugin")().cast<bool>();
    });
}

void PluginManager::EnablePlugin(QString name, bool enable)
{
    if (enable)
        disableList.remove(name);
    else
        disableList.insert(name);

    if (pluginsLoaded)
        SafeRun<void>([=] {
            if (enable)
                wrapper.attr("enable_plugin")(name);
            else
                wrapper.attr("disable_plugin")(name);
        });

    emit PluginStateChanged(name, enable);
}

void PluginManager::UpdatePluginConfig(QString name, const QList<Pi::ConfigItem> &config)
{
    if (!pluginsLoaded)
        return;

    SafeRun<void>([=] {
        QMap<QString, QString> conf;
        for (const auto &i : config)
            conf[i.name] = i.value;
        wrapper.attr("update_plugin_config")(name, conf);
    });
}

bool PluginManager::SearchSubtitle(QString name, QString word, int count)
{
    if (!pluginsLoaded || workerThread)
        return false;

    RunWorker([=] {
        QList<SubtitleEntry> result = wrapper.attr("search_subtitle")(name, word, count).cast<QList<SubtitleEntry>>();
        emit SearchSubtitleFinished(name, std::move(result));
    });
    return true;
}

bool PluginManager::FetchMedia(QString name, int count)
{
    if (!pluginsLoaded || workerThread)
        return false;

    RunWorker([=] {
        QList<MediaEntry> result = wrapper.attr("fetch_media")(name, count).cast<QList<MediaEntry>>();
        emit FetchMediaFinished(name, std::move(result));
    });
    return true;
}

bool PluginManager::SearchMedia(QString name, QString word, int count)
{
    if (!pluginsLoaded || workerThread)
        return false;

    RunWorker([=] {
        QList<MediaEntry> result = wrapper.attr("search_media")(name, word, count).cast<QList<MediaEntry>>();
        emit SearchMediaFinished(name, std::move(result));
    });
    return true;
}
