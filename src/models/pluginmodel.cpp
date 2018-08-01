#include "plugintypes.h"
#include "pluginmodel.h"

PluginModel::PluginModel(QObject *parent) :
    QStandardItemModel(parent)
{
}

PluginModel::~PluginModel()
{
}

Qt::ItemFlags PluginModel::flags(const QModelIndex &index) const
{
    auto flags = QStandardItemModel::flags(index);
    if (index.isValid())
        flags |= Qt::ItemIsUserCheckable;
    return flags;
}

QVariant PluginModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::CheckStateRole) {
        Pi::Plugin plugin = index.data(Qt::UserRole).value<Pi::Plugin>();
        return plugin.enabled ? Qt::Checked : Qt::Unchecked;
    }
    return QStandardItemModel::data(index, role);
}

bool PluginModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::CheckStateRole) {
        Pi::Plugin plugin = index.data(Qt::UserRole).value<Pi::Plugin>();
        plugin.enabled = (value == Qt::Checked);
        QStandardItemModel::setData(index, QVariant::fromValue(plugin), Qt::UserRole);
        return true;
    }
    return QStandardItemModel::setData(index, value, role);
}
