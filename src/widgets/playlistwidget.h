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

    void attachEngine(BakaEngine *baka);

public slots:
    int selectedRow();
    int playingRow();
    int count();
    bool isInPlaylist(QString path);

    void selectRow(int i, bool relative = false);
    void selectIndex(const QModelIndex &index);
    void playRow(int i, bool relative = false);
    void playIndex(const QModelIndex &index);
    void removeRow(int i);
    void removeIndex(const QModelIndex &index);

    void search(QString);
    void shuffle();

    Mpv::PlaylistItem *currentItem();
    Mpv::PlaylistItem *itemAtRow(int row);
    void addItem(QString name, QString path, bool local);
    QModelIndex appendItem(Mpv::PlaylistItem *item);
    void clear();
    void clearNotPlaying();
    void deleteFromDisk(const QModelIndex &index);

signals:
    void currentRowChanged(int row);
    void playlistChanged(QStandardItemModel *);
    void menuVisibilityChanging(bool visible);

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
