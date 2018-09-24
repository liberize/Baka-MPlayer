#include "subtitleprovider.h"
#include "pycast.h"
#include "pluginmanager.h"
#include "worker.h"


void SubtitleProvider::search(QString word, int count)
{
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit searchFinished(result.cast<QList<SubtitleEntry>>());
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        return plugin.attr("search")(word, count);
    });
}

void SubtitleProvider::download(const SubtitleEntry &entry)
{
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit downloadFinished(result.cast<SubtitleEntry>());
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = manager->getModule().attr("SubtitleEntry")(entry.name, entry.url);
        return plugin.attr("download")(obj);
    });
}
