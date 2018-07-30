#include "pluginmanager.h"
#include "util.h"

#include <QDebug>

using namespace Pi;

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
    try {
        py::initialize_interpreter();

        py::module sys = py::module::import("sys");
        const char *scriptsPath = Util::ScriptsPath().toUtf8().constData();
        sys.attr("path").cast<py::list>().append(scriptsPath);

        wrapper = py::module::import("upvwrapper");
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
}

PluginManager::~PluginManager()
{
    try {
        wrapper.release();
        py::finalize_interpreter();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
}

void PluginManager::LoadPlugins(const QList<QString> &disableList)
{
    try {
        wrapper.attr("load_plugins")(Util::PluginsPaths(), disableList);
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
}

QList<Pi::Plugin> PluginManager::GetAllPlugins()
{
    try {
        return wrapper.attr("get_all_plugins")().cast<QList<Plugin>>();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
    return QList<Plugin>();
}

QList<Plugin> PluginManager::GetSubtitlePlugins()
{
    try {
        return wrapper.attr("get_subtitle_plugins")().cast<QList<Plugin>>();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
    return QList<Plugin>();
}

QList<Plugin> PluginManager::GetMediaPlugins()
{
    try {
        return wrapper.attr("get_media_plugins")().cast<QList<Plugin>>();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
    return QList<Plugin>();
}

void PluginManager::EnablePlugin(QString name, bool enable)
{
    try {
        if (enable)
            wrapper.attr("enable_plugin")(name);
        else
            wrapper.attr("disable_plugin")(name);
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
}

void PluginManager::UpdatePluginConfig(QString name, const QList<Pi::ConfigItem> &config)
{
    try {
        QMap<QString, QString> conf;
        for (const auto &i : config)
            conf[i.name] = i.value;
        wrapper.attr("update_plugin_config")(name, conf);
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
}

QList<SubtitleEntry> PluginManager::SearchSubtitle(QString name, QString word, int count)
{
    try {
        return wrapper.attr("search_subtitle")(name, word, count).cast<QList<SubtitleEntry>>();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
    return QList<SubtitleEntry>();
}

QList<MediaEntry> PluginManager::FetchMedia(QString name, int count)
{
    try {
        return wrapper.attr("fetch_media")(name, count).cast<QList<MediaEntry>>();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
    return QList<MediaEntry>();
}

QList<MediaEntry> PluginManager::SearchMedia(QString name, QString word, int count)
{
    try {
        return wrapper.attr("search_media")(name, word, count).cast<QList<MediaEntry>>();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
    return QList<MediaEntry>();
}
