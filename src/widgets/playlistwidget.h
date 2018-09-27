#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QListWidget>
#include <QContextMenuEvent>
#include <QDropEvent>
#include <QAction>
#include <QMouseEvent>
#include <QListView>
#include <QStandardItemModel>

#include "mpvtypes.h"
#include "customlistview.h"

class BakaEngine;
class PlaylistItemDelegate;
class PlaylistProxyModel;

class PlaylistWidget : public CustomListView {
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget *parent = 0);
    ~PlaylistWidget();

    void AttachEngine(BakaEngine *baka);

public slots:
    int selectedRow();
    int playingRow();
    int count();

    void selectRow(int i, bool relative = false);
    void selectIndex(const QModelIndex &index);
    void playRow(int i, bool relative = false);
    void playIndex(const QModelIndex &index);
    void removeRow(int i);
    void removeIndex(const QModelIndex &index);

    void search(const QString&);
    void shuffle();

    void populatePlaylist(QString dir);

    Mpv::PlaylistItem *currentItem();
    void addItem(QString name, QString path, bool local);
    QModelIndex appendItem(Mpv::PlaylistItem *item);
    void clear();
    void clearNotPlaying();
    void deleteFromDisk(const QModelIndex &index);

signals:
    void playlistChanged(QStandardItemModel *);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    BakaEngine *baka;
    QMap<QString, QPersistentModelIndex> pathIndexMap;
    QStandardItemModel *playlistModel = nullptr;
    PlaylistProxyModel *proxyModel = nullptr;
    PlaylistItemDelegate *playlistItemDelegate = nullptr;
    QPersistentModelIndex curPlayingIndex;
};

#endif // PLAYLISTWIDGET_H
