#include "playlistproxymodel.h"
#include "mpvtypes.h"


PlaylistProxyModel::PlaylistProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool PlaylistProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto item = sourceModel()->data(index, Qt::UserRole).value<Mpv::PlaylistItem*>();
    return item->name.contains(filterRegExp());
}

void PlaylistProxyModel::sort(int column, Qt::SortOrder order)
{
    // disable sort
}
