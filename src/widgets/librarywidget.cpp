#include "librarywidget.h"

#include "bakaengine.h"
#include "mpvhandler.h"
#include "ui/mainwindow.h"
#include "delegates/mediaitemdelegate.h"
#include "plugintypes.h"
#include "request.h"
#include "requestmanager.h"

#include <QScrollBar>


LibraryWidget::LibraryWidget(QWidget *parent) :
    CustomListView(parent)
{
    mediaModel = new QStandardItemModel(this);
    mediaItemDelegate = new MediaItemDelegate(this);
    setItemDelegate(mediaItemDelegate);
    setModel(mediaModel);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, [=] (int value) {
        if (value == verticalScrollBar()->maximum())
            emit scrollReachedEnd();
    });
}

LibraryWidget::~LibraryWidget()
{
    clear();
    delete mediaModel;
    delete mediaItemDelegate;
}

void LibraryWidget::attachEngine(BakaEngine *baka)
{
    this->baka = baka;
}

int LibraryWidget::count()
{
    return mediaModel->rowCount();
}

QModelIndex LibraryWidget::appendEntry(MediaEntry *entry)
{
    QStandardItem *item = new QStandardItem;
    item->setData(QVariant::fromValue(entry), Qt::UserRole);
    item->setCheckable(false);
    item->setEditable(false);
    mediaModel->appendRow(item);
    return item->index();
}

void LibraryWidget::clear()
{
    for (int i = 0; i < mediaModel->rowCount(); i++) {
        QModelIndex index = mediaModel->index(i, 0);
        MediaEntry *entry = mediaModel->data(index, Qt::UserRole).value<MediaEntry*>();
        delete entry;
    }
    mediaModel->clear();
}
