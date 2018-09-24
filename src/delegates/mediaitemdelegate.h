#ifndef MEDIAITEMDELEGATE_H
#define MEDIAITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QPixmap>

class MediaItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MediaItemDelegate(QObject *parent = nullptr);
    ~MediaItemDelegate();

    void paint(QPainter * painter,const QStyleOptionViewItem & option,const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // MEDIAITEMDELEGATE_H
