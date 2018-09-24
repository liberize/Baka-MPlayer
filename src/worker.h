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
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

    void run(std::function<py::object()> func);

private:
    void start();

signals:
    void finished(py::object result);

private:
    PluginManager *manager = nullptr;
    QThread *thread = nullptr;
};

#endif // WORKER_H
