#include "onlinewidget.h"

#include "bakaengine.h"
#include "mpvhandler.h"
#include "ui/mainwindow.h"
#include "delegates/mediaitemdelegate.h"
#include "plugintypes.h"
#include "request.h"
#include "requestmanager.h"


OnlineWidget::OnlineWidget(QWidget *parent) :
    CustomListView(parent)
{
    mediaModel = new QStandardItemModel(this);
    mediaItemDelegate = new MediaItemDelegate(this);
    setItemDelegate(mediaItemDelegate);
    setModel(mediaModel);
}

OnlineWidget::~OnlineWidget()
{
    clear();
    delete mediaModel;
    delete mediaItemDelegate;
}

void OnlineWidget::AttachEngine(BakaEngine *baka)
{
    this->baka = baka;
}

QModelIndex OnlineWidget::appendEntry(MediaEntry *entry)
{
    QStandardItem *item = new QStandardItem;
    item->setData(QVariant::fromValue(entry), Qt::UserRole);
    item->setCheckable(false);
    item->setEditable(false);
    mediaModel->appendRow(item);
    return item->index();
}

void OnlineWidget::clear()
{
    for (int i = 0; i < mediaModel->rowCount(); i++) {
        QModelIndex index = mediaModel->index(i, 0);
        MediaEntry *entry = mediaModel->data(index, Qt::UserRole).value<MediaEntry*>();
        delete entry;
    }
    mediaModel->clear();
}
