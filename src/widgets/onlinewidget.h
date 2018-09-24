#ifndef ONLINEWIDGET_H
#define ONLINEWIDGET_H

#include <QListView>
#include <QStandardItemModel>

class BakaEngine;
struct MediaEntry;
class MediaItemDelegate;

class OnlineWidget : public QListView {
    Q_OBJECT

public:
    explicit OnlineWidget(QWidget *parent = 0);
    ~OnlineWidget();

    void AttachEngine(BakaEngine *baka);

    QModelIndex appendEntry(MediaEntry *entry);
    void clear();

signals:
    void mouseMoved(QMouseEvent *event);
    void scrollReachedEnd();

protected:
    void mouseMoveEvent(QMouseEvent *event);

private:
    BakaEngine *baka;
    QStandardItemModel *mediaModel = nullptr;
    MediaItemDelegate *mediaItemDelegate = nullptr;
};

#endif // ONLINEWIDGET_H
