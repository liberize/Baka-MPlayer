#include "customlineedit.h"

#include <QPainter>


CustomLineEdit::CustomLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    SetIcon(QIcon(), QSize());
}

void CustomLineEdit::SetIcon(const QIcon &ic, const QSize &size)
{
    icon = ic;
    iconSize = size;

    if (icon.isNull())
        setTextMargins(1, 1, 1, 1);
    else {
        int padding = (height() - size.height()) / 2;
        setTextMargins(1 + size.width() + padding, 1, 1, 1);
    }
}

void CustomLineEdit::paintEvent(QPaintEvent * event)
{
    QLineEdit::paintEvent(event);
    if (!icon.isNull()) {
        QPainter painter(this);
        int padding = (height() - iconSize.height()) / 2;
        QPixmap pxm = icon.pixmap(iconSize);
        painter.drawPixmap(padding - 2, padding, pxm);
    }
}

void CustomLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return) { // do we need to check Qt::Key_Enter too?
        emit submitted(text());
        event->accept();
    }
    QLineEdit::keyPressEvent(event);
}
