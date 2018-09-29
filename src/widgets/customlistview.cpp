#include "customlistview.h"
#include "util.h"

#include <QPainter>


CustomListView::CustomListView(QWidget *parent) :
    QListView(parent)
{
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
        painter.setFont(QFont(Util::defaultFont(), 13, QFont::Light, true));
        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap , placeholderText);
    }
}
