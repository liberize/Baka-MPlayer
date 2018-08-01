#ifndef PLUGINMODEL_H
#define PLUGINMODEL_H

#include <QStandardItemModel>


class PluginModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit PluginModel(QObject *parent = nullptr);
    ~PluginModel();

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
};

#endif // PLUGINMODEL_H
