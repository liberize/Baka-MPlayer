#include "customlistview.h"

#include <QPainter>
#include <QScrollBar>


CustomListView::CustomListView(QWidget *parent) :
    QListView(parent)
{
    connect(verticalScrollBar(), &QScrollBar::valueChanged, [=] (int value) {
        if (value == verticalScrollBar()->maximum())
            emit scrollReachedEnd();
    });
    connect(selectionModel(), &QItemSelectionModel::currentChanged, [=] (const QModelIndex &current, const QModelIndex &) {
        emit currentRowChanged(current.row());
    });
}

void CustomListView::mouseMoveEvent(QMouseEvent *event)
{
    QListView::mouseMoveEvent(event);
    emit mouseMoved(event);
}

void CustomListView::paintEvent(QPaintEvent *event) {
    QListView::paintEvent(event);
    if ((!model() || model()->rowCount(rootIndex()) == 0) && !placeholderText.isEmpty()) {
        QPainter painter(this->viewport());
        QColor color;
        color.setNamedColor("#959595");
        painter.setPen(QPen(color));
        painter.setFont(QFont("Lucida Grande", 13, QFont::Light, true));
        painter.drawText(rect(), Qt::AlignCenter, placeholderText);
    }
}
