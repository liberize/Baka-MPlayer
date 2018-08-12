#include "pluginmanager.h"
#include "pycast.h"
#include "util.h"
#include "bakaengine.h"
#include "worker.h"

#include <QDebug>


PluginManager::PluginManager(QObject *parent)
    : QObject(parent),
      baka(static_cast<BakaEngine*>(parent))
{
    SafeRun<void>([=] {
        py::initialize_interpreter();

        py::module sys = py::module::import("sys");
        sys.attr("path").attr("insert")(0, py::cast(Util::ScriptsPath()));
        sys.attr("path").attr("insert")(1, py::cast(Util::Path(Util::ScriptsPath(), "packages")));

        py::module os = py::module::import("os");
        os.attr("environ")["TEMP"] = py::cast(baka->tempDir->path());
        os.attr("environ")["TMPDIR"] = py::cast(baka->tempDir->path());
        os.attr("chdir")(py::cast(baka->tempDir->path()));

        module = py::module::import("upv");
    });
}

PluginManager::~PluginManager()
{
    for (auto plugin : plugins)
        delete plugin;

    SafeRun<void>([=] {
        module.release();
        py::finalize_interpreter();
    });
}

Worker *PluginManager::newWorker()
{
    if (busy)
        return nullptr;
    busy = true;
    Worker *worker = new Worker(this);
    connect(worker, &Worker::finished, [=] (py::object) {
        busy = false;
    });
    return worker;
}

void PluginManager::loadPlugins()
{
    auto worker = newWorker();
    connect(worker, &Worker::finished, this, [=] (py::object result) {
        auto objList = result.cast<QList<py::object>>();
        for (auto &obj : objList) {
            Plugin *plugin = nullptr;
            if (obj.attr("is_type")("SubtitleProvider").cast<bool>())
                plugin = new SubtitleProvider(obj, this);
            else if (obj.attr("is_type")("MediaProvider").cast<bool>())
                plugin = new MediaProvider(obj, this);
            else
                plugin = new Plugin(obj, this);

            connect(plugin, &Plugin::enableStateChanged, [=] (bool enable) {
                if (enable)
                    disableList.remove(plugin->getName());
                else
                    disableList.insert(plugin->getName());
            });
            plugins[plugin->getName()] = plugin;
        }
        emit pluginsLoaded(plugins);
        delete worker;
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object manager = module.attr("plugin_manager");
        manager.attr("load_plugins")(Util::PluginsPaths());
        return manager.attr("get_all_plugins")();
    });
}

Plugin *PluginManager::findPlugin(QString name)
{
    return plugins.value(name, nullptr);
}
