#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QThread>
#include <functional>

#include "plugintypes.h"


class Worker : public QObject {
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

    void run(std::function<py::object()> func);

signals:
    void finished(py::object result);

private:
    QThread *thread = nullptr;
};

#endif // WORKER_H
