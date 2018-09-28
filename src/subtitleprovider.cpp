#include "subtitleprovider.h"
#include "pycast.h"
#include "pluginmanager.h"
#include "worker.h"


void SubtitleProvider::search(QString word, int count)
{
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (QVariant result, QString err) {
        if (err.isEmpty())
            emit searchFinished(result.value<QList<SubtitleEntry>>());
        else
            emit error(err);
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = plugin.attr("search")(word, count);
        return QVariant::fromValue(obj.cast<QList<SubtitleEntry>>());
    });
}

void SubtitleProvider::download(const SubtitleEntry &entry)
{
    auto worker = manager->newWorker();
    connect(worker, &Worker::finished, this, [=] (QVariant result, QString err) {
        if (err.isEmpty())
            emit downloadFinished(result.value<SubtitleEntry>());
        else
            emit error(err);
        manager->deleteWorker(worker);
    }, Qt::QueuedConnection);

    worker->run([=] {
        py::object obj = manager->getModule().attr("SubtitleEntry")(entry.name, entry.url);
        obj = plugin.attr("download")(obj);
        return QVariant::fromValue(obj.cast<SubtitleEntry>());
    });
}
