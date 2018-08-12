#include "worker.h"


Worker::Worker(QObject *parent) : QObject(parent)
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

    thread->start();
}
