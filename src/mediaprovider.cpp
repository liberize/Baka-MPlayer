#include "mediaprovider.h"
#include "pycast.h"
#include "pluginmanager.h"
#include "worker.h"

#include <QPersistentModelIndex>


void MediaProvider::fetch(int start, int count)
{
    nextStart = start + count;
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (QVariant result, QString err) {
        if (err.isEmpty())
            emit fetchFinished(result.value<QList<MediaEntry>>());
        else
            emit error(err);
        fetching = false;
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    fetching = true;
    worker->run([=] {
        py::object obj = plugin.attr("fetch")(start, count);
        return QVariant::fromValue(obj.cast<QList<MediaEntry>>());
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
    connect(worker, &Worker::finished, this, [=] (QVariant result, QString err) {
        if (err.isEmpty())
            emit searchFinished(result.value<QList<MediaEntry>>());
        else
            emit error(err);
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = plugin.attr("search")(word, count);
        return QVariant::fromValue(obj.cast<QList<MediaEntry>>());
    });
}

void MediaProvider::download(const MediaEntry &entry, QString what, const QPersistentModelIndex &index)
{
    auto worker = manager->newWorker(what == "cover" ? Worker::Low : Worker::Normal);
    connect(worker, &Worker::finished, this, [=] (QVariant result, QString err) {
        if (err.isEmpty())
            emit downloadFinished(result.value<MediaEntry>(), what, index);
        else
            emit error(err);
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = manager->getModule().attr("MediaEntry")(entry.name, entry.url, entry.options, entry.coverUrl, entry.description);
        obj = plugin.attr("download")(obj, what);
        return QVariant::fromValue(obj.cast<MediaEntry>());
    });
}
