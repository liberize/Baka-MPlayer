#ifndef LIBRARYWIDGET_H
#define LIBRARYWIDGET_H

#include <QListView>
#include <QStandardItemModel>

#include "customlistview.h"

class BakaEngine;
struct MediaEntry;
class MediaItemDelegate;

class LibraryWidget : public CustomListView {
    Q_OBJECT

public:
    explicit LibraryWidget(QWidget *parent = 0);
    ~LibraryWidget();

    void attachEngine(BakaEngine *baka);

    int count();
    QModelIndex appendEntry(MediaEntry *entry);
    void clear();

signals:
    void scrollReachedEnd();

private:
    BakaEngine *baka;
    QStandardItemModel *mediaModel = nullptr;
    MediaItemDelegate *mediaItemDelegate = nullptr;
};

#endif // LIBRARYWIDGET_H
