#include "custompushbutton.h"

#include <QPainter>


CustomPushButton::CustomPushButton(QWidget *parent) :
    QPushButton(parent)
{
}

void CustomPushButton::SetIcon(const QIcon &icon, const QSize &size, int space)
{
    pixmap = icon.pixmap(size);
    spacing = space;
    int x = 10 + size.width() + spacing;
    QString css = QString("%1\nQPushButton {\n\tpadding-left: %2px;\n}").arg(styleSheet(), QString::number(x));
    setStyleSheet(css);
}

void CustomPushButton::paintEvent(QPaintEvent* e)
{
    QPushButton::paintEvent(e);
    if (!pixmap.isNull()) {
        QPainter painter(this);
        int y = (height() - pixmap.height()) / 2;
        painter.drawPixmap(10, y, pixmap);
    }
}
