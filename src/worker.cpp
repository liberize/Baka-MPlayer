#include "worker.h"
#include "pluginmanager.h"


Worker::Worker(Priority p, QObject *parent)
    : QObject(parent),
      manager(static_cast<PluginManager*>(parent)),
      priority(p)
{
    thread = new QThread(this);
}

Worker::~Worker()
{
    delete thread;
}

void Worker::run(std::function<QVariant()> func)
{
    connect(thread, &QThread::started, [=] {
        QString err;
        QVariant var = SafeRun<QVariant>(func, err);
        emit finished(var, err);
        thread->exit();
    });
    manager->runNextWorker();
}

void Worker::start()
{
    thread->start();
}
