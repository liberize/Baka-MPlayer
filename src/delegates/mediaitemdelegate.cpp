#include "mediaitemdelegate.h"
#include "plugintypes.h"
#include "util.h"

#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QEvent>
#include <QDebug>


MediaItemDelegate::MediaItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

MediaItemDelegate::~MediaItemDelegate()
{
}

void MediaItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    if (!index.isValid())
        return;

    painter->save();

    MediaEntry *entry = index.data(Qt::UserRole).value<MediaEntry*>();
    QRect rect = option.rect;

    QColor color;
    color.setNamedColor(option.state.testFlag(QStyle::State_Selected) ? "#565656" : "#202020");
    painter->setPen(QPen(color));
    painter->setBrush(color);
    painter->drawRect(rect);

    const QPixmap &cover = entry->getCover();
    int width = rect.height();
    if (cover.width() > cover.height())
        width = rect.height() * 4 / 3;
    else if (cover.width() < cover.height())
        width = rect.height() * 3 / 4;
    QRect coverRect(rect.x(), rect.y(), width, rect.height());
    painter->drawPixmap(coverRect, cover);
    rect.adjust(width + 10, 10, 10, 10);

    color.setNamedColor("#FFFFFF");
    painter->setPen(QPen(color));

    QRect titleRect(rect);
    painter->setFont(QFont(Util::defaultFont(), 13, QFont::Bold));
    painter->drawText(titleRect, Qt::AlignTop | Qt::AlignLeft, entry->name, &titleRect);

    QRect descRect(rect.x(), titleRect.bottom() + 7, rect.width(), rect.height() - titleRect.height() - 7);
    painter->setFont(QFont(Util::defaultFont(), 11, QFont::Normal));
    QFontMetrics metrics(painter->font());
    QString elidedDesc = metrics.elidedText(entry->description, Qt::ElideRight, descRect.width() - 10);
    painter->drawText(descRect, Qt::AlignTop | Qt::AlignLeft, elidedDesc, &descRect);

    painter->restore();
}

QSize MediaItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize result = QStyledItemDelegate::sizeHint(option, index);
    result.setHeight(60);
    return result;
}
