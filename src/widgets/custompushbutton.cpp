#include "custompushbutton.h"

#include <QPainter>

namespace {
    const int PADDING_LEFT = 10;
}

CustomPushButton::CustomPushButton(QWidget *parent) :
    QPushButton(parent)
{
}

void CustomPushButton::SetIcon(const QIcon &icon, const QSize &size, int space)
{
    pixmap = icon.pixmap(size);
    int x = PADDING_LEFT + size.width() + space;
    QString qss = QString("%1\nQPushButton {\n\tpadding-left: %2px;\n}").arg(styleSheet(), QString::number(x));
    setStyleSheet(qss);
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
