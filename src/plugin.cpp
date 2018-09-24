#include "plugin.h"
#include "pycast.h"
#include "pluginmanager.h"


Plugin::Plugin(const py::object &obj, QObject *parent)
    : QObject(parent),
      plugin(obj),
      manager(static_cast<PluginManager*>(parent))
{
    SafeRun<void>([=] {
        name = obj.attr("name").cast<QString>();
        description = obj.attr("description").cast<QString>();
        config = obj.attr("config").cast<QVector<ConfigItem>>();
        path = obj.attr("path").cast<QString>();
        enabled = obj.attr("enabled").cast<bool>();
        QString iconPath = obj.attr("icon").cast<QString>();
        iconPath = QDir(path).absoluteFilePath(iconPath);
        QFileInfo info(iconPath);
        if (info.exists() && info.isFile())
            icon = QIcon(iconPath);
    });
}

Plugin::~Plugin()
{
}

const QIcon &Plugin::getIcon() const {
    static QIcon defaultIcon(":/img/plugin.svg");
    return icon.isNull() ? defaultIcon : icon;
}

bool Plugin::isSubtitleProvider()
{
    return SafeRun<bool>([=] {
        return plugin.attr("is_type")("SubtitleProvider").cast<bool>();
    });
}

bool Plugin::isMediaProvider()
{
    return SafeRun<bool>([=] {
        return plugin.attr("is_type")("MediaProvider").cast<bool>();
    });
}

void Plugin::setEnabled(bool enable)
{
    SafeRun<void>([=] {
        plugin.attr(enable ? "enable" : "disable")();
    });
    enabled = enable;
    emit enableStateChanged(enabled);
}

void Plugin::updateConfig(const QVector<ConfigItem> &conf)
{
    SafeRun<void>([=] {
        py::dict c;
        for (const auto &i : conf)
            c[py::cast(i.name)] = py::cast(i.value);
        plugin.attr("config") = c;
    });
    config = conf;
    emit configUpdated(config);
}
