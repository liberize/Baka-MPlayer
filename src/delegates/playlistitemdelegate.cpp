#include "playlistitemdelegate.h"
#include "mpvtypes.h"
#include "util.h"

#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QEvent>
#include <QDebug>


PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

PlaylistItemDelegate::~PlaylistItemDelegate()
{
}

void PlaylistItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    if (!index.isValid())
        return;

    painter->save();

    Mpv::PlaylistItem *item = index.data(Qt::UserRole).value<Mpv::PlaylistItem*>();
    QRect rect = option.rect;

    QColor color;
    color.setNamedColor(option.state.testFlag(QStyle::State_Selected) ? "#565656" : "#202020");
    painter->setPen(QPen(color));
    painter->setBrush(color);
    painter->drawRect(rect);

    static QPixmap icon = QIcon(":/img/select_current.svg").pixmap(12, 12);
    int padding = (rect.height() - icon.height()) / 2;
    if (item->playing) {
        QRect iconRect(rect.x() + padding, rect.y() + padding, icon.width(), icon.height());
        painter->drawPixmap(iconRect, icon);
    }
    rect.adjust(icon.width() + padding * 2, 0, 0, 0);

    color.setNamedColor("#FFFFFF");
    painter->setPen(QPen(color));

    painter->setFont(QFont(Util::defaultFont(), 13, item->playing ? QFont::Bold : QFont::Normal));
    QFontMetrics metrics(painter->font());
    QRect nameRect(rect.x(), rect.y() + (rect.height() - metrics.height()) / 2, rect.width(), metrics.height());
    painter->drawText(nameRect, Qt::AlignTop | Qt::AlignLeft, item->name, &nameRect);

    painter->restore();
}

QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize result = QStyledItemDelegate::sizeHint(option, index);
    result.setHeight(30);
    return result;
}
