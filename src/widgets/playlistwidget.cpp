#include "playlistwidget.h"

#include "bakaengine.h"
#include "mpvhandler.h"
#include "ui/mainwindow.h"
#include "util.h"
#include "models/playlistproxymodel.h"
#include "delegates/playlistitemdelegate.h"

#include <QMenu>
#include <QFont>
#include <QMessageBox>
#include <QMimeData>
#include <QDebug>

#include <algorithm>

PlaylistWidget::PlaylistWidget(QWidget *parent) :
    CustomListView(parent)
{
    playlistModel = new QStandardItemModel(this);
    playlistItemDelegate = new PlaylistItemDelegate(this);
    proxyModel = new PlaylistProxyModel(this);
    proxyModel->setSourceModel(playlistModel);
    setItemDelegate(playlistItemDelegate);
    setModel(proxyModel);
}

PlaylistWidget::~PlaylistWidget()
{
    clear();
    delete proxyModel;
    delete playlistModel;
    delete playlistItemDelegate;
}

void PlaylistWidget::attachEngine(BakaEngine *baka)
{
    this->baka = baka;

    connect(baka->mpv, &MpvHandler::pathChanged, [=] (QString path) {
        populatePlaylist(path);
    });

    connect(baka->mpv, &MpvHandler::fileChanged, [=] (QString file, QString title, const QMap<QString, QString> &options) {
        QString path = baka->mpv->getPath() + file;
        auto iter = pathIndexMap.find(path);
        QPersistentModelIndex index;
        if (iter == pathIndexMap.end()) {
            auto item = new Mpv::PlaylistItem { title.isEmpty() ? file : title, path, options, !baka->mpv->getPath().isEmpty(), true };
            index = appendItem(item);
        } else {
            index = *iter;
            auto item = index.data(Qt::UserRole).value<Mpv::PlaylistItem*>();
            if (!title.isEmpty())
                item->name = title;
            item->options = options;
            item->playing = true;
        }
        if (curPlayingIndex.isValid() && curPlayingIndex != index) {
            auto item = curPlayingIndex.data(Qt::UserRole).value<Mpv::PlaylistItem*>();
            item->playing = false;
            emit dataChanged(curPlayingIndex, curPlayingIndex);
        }
        curPlayingIndex = index;
        selectIndex(index);
    });
}

QModelIndex PlaylistWidget::appendItem(Mpv::PlaylistItem *i)
{
    QStandardItem *item = new QStandardItem;
    item->setData(QVariant::fromValue(i), Qt::UserRole);
    item->setCheckable(false);
    item->setEditable(false);
    playlistModel->appendRow(item);
    pathIndexMap[i->path] = item->index();
    return item->index();
}

void PlaylistWidget::clear()
{
    int rowCount = playlistModel->rowCount();
    for (int i = 0; i < rowCount; i++) {
        QModelIndex index = playlistModel->index(i, 0);
        Mpv::PlaylistItem *item = playlistModel->data(index, Qt::UserRole).value<Mpv::PlaylistItem*>();
        delete item;
    }
    playlistModel->clear();
    pathIndexMap.clear();
}

void PlaylistWidget::clearNotPlaying()
{
    if (!curPlayingIndex.isValid()) {
        clear();
    } else {
        while (playlistModel->rowCount() > 1) {
            int row = curPlayingIndex.row() != 0 ? 0 : 1;
            QModelIndex index = playlistModel->index(row, 0);
            Mpv::PlaylistItem *item = playlistModel->data(index, Qt::UserRole).value<Mpv::PlaylistItem*>();
            pathIndexMap.remove(item->path);
            delete item;
            playlistModel->removeRow(row);
        }
    }
}

void PlaylistWidget::populatePlaylist(QString dir)
{
    if (dir.isEmpty())
        return;

    QDir root(dir);
    QStringList filter = Mpv::media_filetypes;
    QFileInfoList flist = root.entryInfoList(filter, QDir::Files);
    for (auto &i : flist) {
        auto item = new Mpv::PlaylistItem { i.fileName(), dir + i.fileName(), QMap<QString, QString>(), true, false };
        if (!pathIndexMap.contains(item->path))
            appendItem(item);
    }
    emit playlistChanged(playlistModel);
}

Mpv::PlaylistItem *PlaylistWidget::currentItem()
{
    QModelIndex index = curPlayingIndex.isValid() ? (QModelIndex)curPlayingIndex : playlistModel->index(0, 0);
    return index.isValid() ? index.data(Qt::UserRole).value<Mpv::PlaylistItem*>() : nullptr;
}

void PlaylistWidget::addItem(QString name, QString path, bool local)
{
    auto item = new Mpv::PlaylistItem { name, path, QMap<QString, QString>(), local, false };
    auto iter = pathIndexMap.find(path);
    if (iter == pathIndexMap.end())
        appendItem(item);
    else
        selectIndex(*iter);
}

int PlaylistWidget::selectedRow()
{
    return selectionModel()->selectedRows().isEmpty() ? -1 : selectionModel()->selectedRows().back().row();
}

int PlaylistWidget::playingRow()
{
    return curPlayingIndex.isValid() ? curPlayingIndex.row() : -1;
}

int PlaylistWidget::count()
{
    return playlistModel->rowCount();
}

void PlaylistWidget::selectRow(int i, bool relative)
{
    int row = relative ? selectedRow() + i : i;
    row = qMin(qMax(row, 0), playlistModel->rowCount() - 1);
    selectIndex(playlistModel->index(row, 0));
}

void PlaylistWidget::selectIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    setCurrentIndex(index);
    selectionModel()->clear();
    selectionModel()->select(index, QItemSelectionModel::Select);
    scrollTo(index);
}

void PlaylistWidget::playRow(int i, bool relative)
{
    int row = relative ? playingRow() + i : i;
    row = qMin(qMax(row, 0), playlistModel->rowCount() - 1);
    playIndex(playlistModel->index(row, 0));
}

void PlaylistWidget::playIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    auto item = index.data(Qt::UserRole).value<Mpv::PlaylistItem*>();
    if (baka->mpv->playFile(item->path, item->name, item->options)) {
        scrollTo(index);
    } else {
        playIndex(playlistModel->index(index.row() + 1, 0));
        removeIndex(index);
    }
}

void PlaylistWidget::removeRow(int i)
{
    int row = qMin(qMax(i, 0), playlistModel->rowCount() - 1);
    removeIndex(playlistModel->index(row, 0));
}

void PlaylistWidget::removeIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    Mpv::PlaylistItem *item = index.data(Qt::UserRole).value<Mpv::PlaylistItem*>();
    pathIndexMap.remove(item->path);
    delete item;
    playlistModel->removeRow(index.row());
}

void PlaylistWidget::search(QString s)
{
    proxyModel->setFilterFixedString(s);
}

void PlaylistWidget::shuffle()
{
    int rowCount = playlistModel->rowCount();
    if (rowCount == 0)
        return;

    int selectedRow = -1;
    Mpv::PlaylistItem *selected = nullptr;
    if (!selectionModel()->selectedRows().isEmpty())
        selected = selectionModel()->selectedRows().back().data(Qt::UserRole).value<Mpv::PlaylistItem*>();

    for (int i = 0; i < rowCount; i++) {
        int r = Util::randInt(i, rowCount - 1);
        auto row = playlistModel->takeRow(r);
        playlistModel->insertRow(i, row);
        auto item = row[0]->data(Qt::UserRole).value<Mpv::PlaylistItem*>();
        pathIndexMap[item->path] = playlistModel->index(i, 0);
        if (item->playing)
            curPlayingIndex = playlistModel->index(i, 0);
        if (item == selected)
            selectedRow = i;
    }

    if (selectedRow != -1) {
        selectRow(selectedRow);
        emit currentRowChanged(selectedRow);
    }
}

void PlaylistWidget::deleteFromDisk(const QModelIndex &index)
{
    auto item = index.data(Qt::UserRole).value<Mpv::PlaylistItem*>();
    if (!item->local)
        return;

    QString r = item->path.left(item->path.lastIndexOf('.') + 1); // get file root (no extension)
    // check and remove all subtitle_files with the same root as the video
    for (auto ext : Mpv::subtitle_filetypes) {
        QFile subf(r + ext.mid(2));
        if (subf.exists() && QMessageBox::question(parentWidget(), tr("Delete sub-file?"),
            tr("Would you like to delete the associated sub file [%0]?").arg(subf.fileName())) == QMessageBox::Yes)
            subf.remove();
    }
    // check and remove all external subtitle files in the video
    for (auto track : baka->mpv->getFileInfo().tracks) {
        if (track.external) {
            QFile subf(track.external_filename);
            if (subf.exists() && QMessageBox::question(parentWidget(), tr("Delete external sub-file?"),
                tr("Would you like to delete the associated sub file [%0]?").arg(subf.fileName())) == QMessageBox::Yes)
                subf.remove();
        }
    }
    // remove the actual file
    QFile f(item->path);
    f.remove();
    removeIndex(index);
}

void PlaylistWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid())
        return;

    Mpv::PlaylistItem *item = index.data(Qt::UserRole).value<Mpv::PlaylistItem*>();
    QMenu *menu = new QMenu();
    connect(menu->addAction(tr("R&emove from Playlist")), &QAction::triggered, [=] {    // Playlist: Remove from playlist (right-click)
        if (item->playing)
            playIndex(playlistModel->index(index.row() + 1, 0));
        removeIndex(index);
    });
    if (item->local) {
        connect(menu->addAction(tr("&Delete from Disk")), &QAction::triggered, [=] {        // Playlist: Delete from Disk (right-click)
            deleteFromDisk(index);
        });
        connect(menu->addAction(tr("&Show in Folder")), &QAction::triggered, [=] {        // Playlist: Show in Folder (right-click)
            QFileInfo fi(item->path);
            Util::showInFolder(fi.absolutePath(), fi.fileName());
        });
    }
    menu->exec(viewport()->mapToGlobal(event->pos()));
    delete menu;
}

void PlaylistWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void PlaylistWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) { // urls
        for (QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                QFileInfo fi(url.toLocalFile());
                addItem(fi.fileName(), fi.absoluteFilePath(), true);
            } else
                addItem(url.url(), url.url(), false);
        }
    }
    event->acceptProposedAction();
}
