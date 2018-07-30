#ifndef PLUGINTYPES_H
#define PLUGINTYPES_H

#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/stl.h"
namespace py = pybind11;

#include <QString>
#include <QMap>
#include <QHash>
#include <QVector>
#include <QList>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <functional>


namespace Pi {

struct ConfigItem {
    enum Type { UNKNOWN, STR, INT, BOOL, FLOAT };
    QString name;
    QString title;
    Type type;
    std::function<bool(QString)> validator;
    QString value;

    ConfigItem &operator =(const py::object &obj) {
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
        return *this;
    }
    bool operator <(const ConfigItem &r) const { return name < r.name; }
};

struct Plugin {
    QString name;
    QIcon icon;
    QString description;
    QList<ConfigItem> config;
    QString path;
    bool enabled;

    Plugin &operator =(const py::object &obj) {
        name = obj.attr("name").cast<QString>();
        description = obj.attr("description").cast<QString>();
        config = obj.attr("config").cast<QList<ConfigItem>>();
        path = obj.attr("path").cast<QString>();
        enabled = obj.attr("enabled").cast<bool>();
        QString iconPath = obj.attr("icon").cast<QString>();
        iconPath = QDir(path).absoluteFilePath(iconPath);
        QFileInfo info(iconPath);
        if (!info.exists() || !info.isFile())
            iconPath = ":/img/plugin.svg";
        icon = QIcon(iconPath);
        return *this;
    }
};

struct MediaEntry {
    QString name;
    QString url;
    QString cover;
    QString description;

    MediaEntry &operator =(const py::object &obj) {
        name = obj.attr("name").cast<QString>();
        url = obj.attr("url").cast<QString>();
        cover = obj.attr("cover").cast<QString>();
        description = obj.attr("description").cast<QString>();
        return *this;
    }
};

struct SubtitleEntry {
    QString name;
    QString url;

    SubtitleEntry &operator =(const py::object &obj) {
        name = obj.attr("name").cast<QString>();
        url = obj.attr("url").cast<QString>();
        return *this;
    }
};

}

namespace pybind11 {
namespace detail {

// basic qt type casters

template <> struct type_caster<QString> {

    bool load(handle src, bool/* convert*/) {
        if (!isinstance<str>(src))
            return false;
        auto s = reinterpret_borrow<str>(src);

        object temp = s;
        if (PyUnicode_Check(s.ptr())) {
            temp = reinterpret_steal<object>(PyUnicode_AsUTF8String(s.ptr()));
            if (!temp)
                pybind11_fail("Unable to extract string contents! (encoding issue)");
        }
        char *buffer;
        ssize_t length;
        if (PYBIND11_BYTES_AS_STRING_AND_SIZE(temp.ptr(), &buffer, &length))
            pybind11_fail("Unable to extract string contents! (invalid type)");

        value = QString::fromUtf8(buffer, length);
        return true;
    }

    static handle cast(const QString &src, return_value_policy/* policy*/, handle/* parent*/) {
        auto b = src.toUtf8();
        str s(b.constData(), b.size());
        return s.release();
    }

    PYBIND11_TYPE_CASTER(QString, _("str"));
};

template <typename Type, typename Key, typename Value> struct qmap_caster {
    using key_conv   = make_caster<Key>;
    using value_conv = make_caster<Value>;

    bool load(handle src, bool convert) {
        if (!isinstance<dict>(src))
            return false;
        auto d = reinterpret_borrow<dict>(src);
        value.clear();
        for (auto it : d) {
            key_conv kconv;
            value_conv vconv;
            if (!kconv.load(it.first.ptr(), convert) ||
                !vconv.load(it.second.ptr(), convert))
                return false;
            value[cast_op<Key &&>(std::move(kconv))] = cast_op<Value &&>(std::move(vconv));
        }
        return true;
    }

    template <typename T>
    static handle cast(T &&src, return_value_policy policy, handle parent) {
        dict d;
        for (auto &&it = src.begin(); it != src.end(); ++it) {
            auto key = reinterpret_steal<object>(key_conv::cast(forward_like<T>(it.key()), policy, parent));
            auto value = reinterpret_steal<object>(value_conv::cast(forward_like<T>(*it), policy, parent));
            if (!key || !value)
                return handle();
            d[key] = value;
        }
        return d.release();
    }

    PYBIND11_TYPE_CASTER(Type, _("Dict[") + key_conv::name() + _(", ") + value_conv::name() + _("]"));
};

template <typename Type, typename Value> struct qlist_caster {
    using value_conv = make_caster<Value>;

    bool load(handle src, bool convert) {
        if (!isinstance<sequence>(src))
            return false;
        auto s = reinterpret_borrow<sequence>(src);
        value.clear();
        reserve_maybe(s, &value);
        for (auto it : s) {
            value_conv conv;
            if (!conv.load(it, convert))
                return false;
            value.push_back(cast_op<Value &&>(std::move(conv)));
        }
        return true;
    }

private:
    template <typename T = Type,
              enable_if_t<std::is_same<decltype(std::declval<T>().reserve(0)), void>::value, int> = 0>
    void reserve_maybe(sequence s, Type *) { value.reserve(s.size()); }
    void reserve_maybe(sequence, void *) { }

public:
    template <typename T>
    static handle cast(T &&src, return_value_policy policy, handle parent) {
        list l(src.size());
        size_t index = 0;
        for (auto &&value : src) {
            auto value_ = reinterpret_steal<object>(value_conv::cast(forward_like<T>(value), policy, parent));
            if (!value_)
                return handle();
            PyList_SET_ITEM(l.ptr(), (ssize_t) index++, value_.release().ptr()); // steals a reference
        }
        return l.release();
    }

    PYBIND11_TYPE_CASTER(Type, _("List[") + value_conv::name() + _("]"));
};

template <typename Type> struct type_caster<QVector<Type>>
 : qlist_caster<QVector<Type>, Type> { };

template <typename Type> struct type_caster<QList<Type>>
 : qlist_caster<QList<Type>, Type> { };


template <typename Key, typename Value> struct type_caster<QMap<Key, Value>>
  : qmap_caster<QMap<Key, Value>, Key, Value> { };

template <typename Key, typename Value> struct type_caster<QHash<Key, Value>>
  : qmap_caster<QHash<Key, Value>, Key, Value> { };


// plugin type casters

template <typename Type> struct plugin_caster {

    bool load(handle src, bool/* convert*/) {
        value = reinterpret_borrow<object>(src);
        return true;
    }

    template <typename T>
    static handle cast(T &&/*src*/, return_value_policy/* policy*/, handle/* parent*/) {
        object s;
        pybind11_fail("undefined c++ to python cast");
        return s.release();
    }

    PYBIND11_TYPE_CASTER(Type, _("object"));
};

template <> struct type_caster<Pi::ConfigItem>
  : plugin_caster<Pi::ConfigItem> { };

template <> struct type_caster<Pi::Plugin>
  : plugin_caster<Pi::Plugin> { };

template <> struct type_caster<Pi::MediaEntry>
  : plugin_caster<Pi::MediaEntry> { };

template <> struct type_caster<Pi::SubtitleEntry>
  : plugin_caster<Pi::SubtitleEntry> { };

}
}

Q_DECLARE_METATYPE(Pi::ConfigItem)
Q_DECLARE_METATYPE(Pi::Plugin)
Q_DECLARE_METATYPE(Pi::MediaEntry)
Q_DECLARE_METATYPE(Pi::SubtitleEntry)

#endif // PLUGINTYPES_H
