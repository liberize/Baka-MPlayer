#include "mediaprovider.h"
#include "pycast.h"
#include "pluginmanager.h"
#include "worker.h"

#include <QPersistentModelIndex>


void MediaProvider::fetch(int start, int count)
{
    nextStart = start + count;
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit fetchFinished(result.cast<QList<MediaEntry>>());
        fetching = false;
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    fetching = true;
    worker->run([=] {
        return plugin.attr("fetch")(start, count);
    });
}

void MediaProvider::fetchNext(int count)
{
    if (!fetching)
        fetch(nextStart, count);
}

void MediaProvider::search(QString word, int count)
{
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit searchFinished(result.cast<QList<MediaEntry>>());
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        return plugin.attr("search")(word, count);
    });
}

void MediaProvider::download(const MediaEntry &entry, QString what, const QPersistentModelIndex &index)
{
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (py::object result) {
        emit downloadFinished(result.cast<MediaEntry>(), what, index);
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = manager->getModule().attr("MediaEntry")(entry.name, entry.url, entry.options, entry.coverUrl, entry.description);
        return plugin.attr("download")(obj, what);
    });
}
