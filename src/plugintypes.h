#ifndef PLUGINTYPES_H
#define PLUGINTYPES_H

#pragma push_macro("slots")
#undef slots

#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/stl.h"
namespace py = pybind11;

#pragma pop_macro("slots")

#include "util.h"

#include <QString>
#include <QDir>
#include <QIcon>
#include <QPixmap>
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
    QMap<QString, QString> options;
    QString coverUrl;
    QPixmap cover;
    QString description;

    MediaEntry() {}
    MediaEntry(const py::object &obj) {
        operator =(obj);
    }

    MediaEntry &operator =(const py::object &obj) {
        SafeRun<void>([=] {
            name = obj.attr("name").cast<QString>();
            url = obj.attr("url").cast<QString>();
            options = obj.attr("options").cast<QMap<QString, QString>>();
            coverUrl = obj.attr("cover").cast<QString>();
            description = obj.attr("description").cast<QString>();
            QString localFile = Util::ToLocalFile(coverUrl);
            if (!localFile.isEmpty())
                cover = QPixmap(localFile);
        });
        return *this;
    }

    const QPixmap &getCover() {
        static QPixmap defaultCover = QIcon(":/img/cover.svg").pixmap(60, 60);
        return cover.isNull() ? defaultCover : cover;
    }
};

struct SubtitleEntry {
    QString name;
    QString url;

    SubtitleEntry() {}
    SubtitleEntry(const py::object &obj) {
        operator =(obj);
    }

    SubtitleEntry &operator =(const py::object &obj) {
        SafeRun<void>([=] {
            name = obj.attr("name").cast<QString>();
            url = obj.attr("url").cast<QString>();
        });
        return *this;
    }
};

Q_DECLARE_METATYPE(MediaEntry*)
Q_DECLARE_METATYPE(py::object)

#endif // PLUGINTYPES_H
