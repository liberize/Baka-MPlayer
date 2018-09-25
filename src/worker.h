#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QThread>
#include <functional>

#include "plugintypes.h"

class PluginManager;

class Worker : public QObject {
    friend class PluginManager;
    Q_OBJECT

public:
    enum Priority { Normal = 0, Low = -1, High = 1 };

public:
    explicit Worker(Priority p, QObject *parent = nullptr);
    ~Worker();

    void run(std::function<py::object()> func);
    Priority getPriority() const { return priority; }

private:
    void start();

signals:
    void finished(py::object result);

private:
    PluginManager *manager = nullptr;
    QThread *thread = nullptr;
    Priority priority = Normal;
};

#endif // WORKER_H
