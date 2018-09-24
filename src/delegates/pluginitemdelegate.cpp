#include "pluginitemdelegate.h"
#include "plugin.h"

#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QEvent>
#include <QDebug>


PluginItemDelegate::PluginItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

PluginItemDelegate::~PluginItemDelegate()
{
}

void PluginItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    if (!index.isValid())
        return;

    painter->save();

    Plugin *plugin = index.data(Qt::UserRole).value<Plugin*>();
    QRect rect = option.rect.adjusted(35, 0, 0, 0);

    QColor color;
    color.setNamedColor(option.state.testFlag(QStyle::State_Selected) ? "#565656" : "#3B3B3B");
    painter->setPen(QPen(color));
    painter->setBrush(color);
    painter->drawRect(rect);

    rect.adjust(10, 10, -10, -10);

    int iconSize = rect.height();
    QRect iconRect(rect.x(), rect.y(), iconSize, iconSize);
    plugin->getIcon().paint(painter, iconRect);

    rect.adjust(iconSize + 15, 0, 0, 0);

    color.setNamedColor("#FFFFFF");
    painter->setPen(QPen(color));

    QRect titleRect(rect);
    painter->setFont(QFont("Lucida Grande", 13, QFont::Bold));
    painter->drawText(titleRect, Qt::AlignTop | Qt::AlignLeft, plugin->getName(), &titleRect);

    QRect descRect(rect.x(), titleRect.bottom() + 7, rect.width(), rect.height() - titleRect.height() - 7);
    painter->setFont(QFont("Lucida Grande", 11, QFont::Normal));
    painter->drawText(descRect, Qt::AlignTop | Qt::AlignLeft, plugin->getDescription(), &descRect);

    painter->restore();
}

QSize PluginItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(320, 60);
}
