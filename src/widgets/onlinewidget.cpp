#include "onlinewidget.h"

#include "bakaengine.h"
#include "mpvhandler.h"
#include "ui/mainwindow.h"

#include <QListWidgetItem>


OnlineWidget::OnlineWidget(QWidget *parent) :
    QListWidget(parent)
{
}

OnlineWidget::~OnlineWidget()
{
}

void OnlineWidget::AttachEngine(BakaEngine *baka)
{
    this->baka = baka;
}

void OnlineWidget::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved();
    QListWidget::mouseMoveEvent(event);
}
