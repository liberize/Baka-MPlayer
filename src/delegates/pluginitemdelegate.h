#ifndef PLUGINITEMDELEGATE_H
#define PLUGINITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QStandardItemModel>

class PluginItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PluginItemDelegate(QObject *parent = nullptr);
    ~PluginItemDelegate();

    void paint(QPainter * painter,const QStyleOptionViewItem & option,const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // PLUGINITEMDELEGATE_H
