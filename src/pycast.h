#ifndef PYCAST_H
#define PYCAST_H

#pragma push_macro("slots")
#undef slots

#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/stl.h"
namespace py = pybind11;

#pragma pop_macro("slots")

#include <QByteArray>
#include <QMap>
#include <QHash>
#include <QVector>
#include <QList>
#include <QDebug>

#include "plugintypes.h"
#include "plugin.h"
#include "subtitleprovider.h"
#include "mediaprovider.h"


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

template <> struct type_caster<QByteArray> {

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

        value = QByteArray(buffer, length);
        return true;
    }

    static handle cast(const QByteArray &src, return_value_policy/* policy*/, handle/* parent*/) {
        str s(src.constData(), src.size());
        return s.release();
    }

    PYBIND11_TYPE_CASTER(QByteArray, _("str"));
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

template <typename Type> struct plugin_struct_caster {

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

template <> struct type_caster<ConfigItem>
  : plugin_struct_caster<ConfigItem> { };

template <> struct type_caster<MediaEntry>
  : plugin_struct_caster<MediaEntry> { };

template <> struct type_caster<SubtitleEntry>
  : plugin_struct_caster<SubtitleEntry> { };

}
}

#endif // PYCAST_H
