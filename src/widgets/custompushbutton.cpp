#include "custompushbutton.h"

#include <QPainter>

namespace {
    const int PADDING_LEFT = 10;
}

CustomPushButton::CustomPushButton(QWidget *parent) :
    QPushButton(parent)
{
}

void CustomPushButton::setIcon(const QIcon &icon, const QSize &size)
{
    pixmap = icon.pixmap(size);
}

void CustomPushButton::paintEvent(QPaintEvent* e)
{
    QPushButton::paintEvent(e);
    if (!pixmap.isNull()) {
        QPainter painter(this);
        int y = (height() - pixmap.height()) / 2;
        painter.drawPixmap(PADDING_LEFT, y, pixmap);
    }
}
