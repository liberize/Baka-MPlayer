#ifndef PLUGINTYPES_H
#define PLUGINTYPES_H

#pragma push_macro("slots")
#undef slots

#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/stl.h"
namespace py = pybind11;

#pragma pop_macro("slots")

#include <QString>
#include <QDir>
#include <QIcon>
#include <QDebug>
#include <functional>


template <typename T>
T SafeRun(std::function<T()> func) {
    try {
        return func();
    } catch (std::runtime_error e) {
        qDebug() << e.what();
    }
    return T();
}

struct ConfigItem {
    enum Type { UNKNOWN, STR, INT, BOOL, FLOAT };
    QString name;
    QString title;
    Type type = STR;
    std::function<bool(QString)> validator;
    QString value;

    ConfigItem &operator =(const py::object &obj) {
        SafeRun<void>([=] {
            name = obj.attr("name").cast<QString>();
            title = obj.attr("title").cast<QString>();
            type = UNKNOWN;
            QString n = obj.attr("type").attr("__name__").cast<QString>();
            if (n == "str")
                type = STR;
            else if (n == "int")
                type = INT;
            else if (n == "bool")
                type = BOOL;
            else if (n == "float")
                type = FLOAT;
            validator = [=] (QString s) -> bool {
                return obj.attr("validator")(s).cast<bool>();
            };
            value = obj.attr("value").cast<QString>();
        });
        return *this;
    }
    bool operator <(const ConfigItem &r) const { return name < r.name; }
};

struct MediaEntry {
    QString name;
    QString url;
    QString cover;
    QString description;
    QString downloader;

    MediaEntry &operator =(const py::object &obj) {
        SafeRun<void>([=] {
            name = obj.attr("name").cast<QString>();
            url = obj.attr("url").cast<QString>();
            cover = obj.attr("cover").cast<QString>();
            description = obj.attr("description").cast<QString>();
            downloader = obj.attr("downloader").cast<QString>();
        });
        return *this;
    }
};

struct SubtitleEntry {
    QString name;
    QString url;
    QString downloader;

    SubtitleEntry &operator =(const py::object &obj) {
        SafeRun<void>([=] {
            name = obj.attr("name").cast<QString>();
            url = obj.attr("url").cast<QString>();
            downloader = obj.attr("downloader").cast<QString>();
        });
        return *this;
    }
};

Q_DECLARE_METATYPE(ConfigItem)
Q_DECLARE_METATYPE(MediaEntry)
Q_DECLARE_METATYPE(SubtitleEntry)
Q_DECLARE_METATYPE(QList<MediaEntry>)
Q_DECLARE_METATYPE(QList<SubtitleEntry>)
Q_DECLARE_METATYPE(py::object)

#endif // PLUGINTYPES_H
