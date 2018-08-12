#include "mediaprovider.h"
#include "pycast.h"
#include "pluginmanager.h"
#include "worker.h"


bool MediaProvider::fetch(int start, int count)
{
    auto worker = manager->newWorker();
    if (!worker)
        return false;

    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit fetchFinished(result.cast<QList<MediaEntry>>());
        delete worker;
    }, Qt::QueuedConnection);

    worker->run([=] {
        return plugin.attr("fetch")(start, count);
    });
    return true;
}

bool MediaProvider::search(QString word, int count)
{
    auto worker = manager->newWorker();
    if (!worker)
        return false;

    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit searchFinished(result.cast<QList<MediaEntry>>());
        delete worker;
    }, Qt::QueuedConnection);

    worker->run([=] {
        return plugin.attr("search")(word, count);
    });
    return true;
}

bool MediaProvider::download(const MediaEntry &entry)
{
    auto worker = manager->newWorker();
    if (!worker)
        return false;

    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit downloadFinished(result.cast<MediaEntry>());
        delete worker;
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = manager->getModule().attr("MediaEntry")(entry.name, entry.url, entry.cover, entry.description, entry.downloader);
        return plugin.attr("download")(obj);
    });
    return true;
}
