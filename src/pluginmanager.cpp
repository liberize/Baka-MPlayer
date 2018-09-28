#include "pluginmanager.h"
#include "pycast.h"
#include "util.h"
#include "bakaengine.h"
#include "ui/mainwindow.h"

#include <QDebug>


PluginManager::PluginManager(QObject *parent)
    : QObject(parent),
      baka(static_cast<BakaEngine*>(parent))
{
    SafeRun<void>([=] {
        py::initialize_interpreter();

        py::module sys = py::module::import("sys");
        sys.attr("path").attr("insert")(0, py::cast(Util::scriptsPath()));
        sys.attr("path").attr("insert")(1, py::cast(Util::path(Util::scriptsPath(), "packages")));

        py::module os = py::module::import("os");
        os.attr("environ")["TEMP"] = py::cast(baka->tempDir->path());
        os.attr("environ")["TMPDIR"] = py::cast(baka->tempDir->path());
        os.attr("chdir")(py::cast(baka->tempDir->path()));

        module = py::module::import("upv");
        module.attr("input") = py::cpp_function([=] (QString prompt) {
            QString inputStr;
            QMetaObject::invokeMethod(baka->window,
                                      "getInput",
                                      QThread::currentThread() == baka->window->thread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                                      Q_RETURN_ARG(QString, inputStr),
                                      Q_ARG(QString, "Plugin Input"),
                                      Q_ARG(QString, prompt));
            return inputStr;
        });
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

Worker *PluginManager::newWorker(Worker::Priority priority)
{
    Worker *worker = new Worker(priority, this);
    int i = 0;
    for (; i < workerQueue.count(); i++) {
        if (priority > workerQueue[i]->getPriority())
            break;
    }
    workerQueue.insert(i, worker);
    return worker;
}

void PluginManager::runNextWorker()
{
    if (!busy && !workerQueue.isEmpty()) {
        busy = true;
        workerQueue.dequeue()->start();
    }
}

void PluginManager::deleteWorker(Worker *worker)
{
    delete worker;
    busy = false;
    runNextWorker();
}

void PluginManager::clearWorkers(Worker::Priority maxPriority)
{
    QMutableListIterator<Worker*> i(workerQueue);
    while (i.hasNext()) {
        if (i.next()->getPriority() <= maxPriority)
            i.remove();
    }
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
        deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object manager = module.attr("plugin_manager");
        manager.attr("load_plugins")(Util::pluginsPaths());
        return manager.attr("get_all_plugins")();
    });
}

Plugin *PluginManager::findPlugin(QString name)
{
    return plugins.value(name, nullptr);
}
