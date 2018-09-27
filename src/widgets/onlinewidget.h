#ifndef ONLINEWIDGET_H
#define ONLINEWIDGET_H

#include <QListView>
#include <QStandardItemModel>

#include "customlistview.h"

class BakaEngine;
struct MediaEntry;
class MediaItemDelegate;

class OnlineWidget : public CustomListView {
    Q_OBJECT

public:
    explicit OnlineWidget(QWidget *parent = 0);
    ~OnlineWidget();

    void AttachEngine(BakaEngine *baka);

    QModelIndex appendEntry(MediaEntry *entry);
    void clear();

private:
    BakaEngine *baka;
    QStandardItemModel *mediaModel = nullptr;
    MediaItemDelegate *mediaItemDelegate = nullptr;
};

#endif // ONLINEWIDGET_H
