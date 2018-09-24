#ifndef PLAYLISTFILTERPROXYMODEL_H
#define PLAYLISTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class PlaylistProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    PlaylistProxyModel(QObject *parent = 0);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
};

#endif // PLAYLISTFILTERPROXYMODEL_H
