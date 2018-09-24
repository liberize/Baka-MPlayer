#include "worker.h"
#include "pluginmanager.h"


Worker::Worker(QObject *parent)
    : QObject(parent),
      manager(static_cast<PluginManager*>(parent))
{
    thread = new QThread(this);
}

Worker::~Worker()
{
    delete thread;
}

void Worker::run(std::function<py::object()> func)
{
    connect(thread, &QThread::started, [=] {
        emit finished(SafeRun<py::object>(func));
        thread->exit();
    });
    manager->runNextWorker();
}

void Worker::start()
{
    thread->start();
}
