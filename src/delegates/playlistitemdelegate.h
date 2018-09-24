#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QPixmap>

class PlaylistItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit PlaylistItemDelegate(QObject *parent = nullptr);
    ~PlaylistItemDelegate();

    void paint(QPainter * painter,const QStyleOptionViewItem & option,const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // PLAYLISTITEMDELEGATE_H
