#include "subtitleprovider.h"
#include "pycast.h"
#include "pluginmanager.h"
#include "worker.h"


bool SubtitleProvider::search(QString word, int count)
{
    auto worker = manager->newWorker();
    if (!worker)
        return false;

    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit searchFinished(result.cast<QList<SubtitleEntry>>());
        delete worker;
    }, Qt::QueuedConnection);

    worker->run([=] {
        return plugin.attr("search")(word, count);
    });
    return true;
}

bool SubtitleProvider::download(const SubtitleEntry &entry)
{
    auto worker = manager->newWorker();
    if (!worker)
        return false;

    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit downloadFinished(result.cast<SubtitleEntry>());
        delete worker;
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = manager->getModule().attr("SubtitleEntry")(entry.name, entry.url, entry.downloader);
        return plugin.attr("download")(obj);
    });
    return true;
}
