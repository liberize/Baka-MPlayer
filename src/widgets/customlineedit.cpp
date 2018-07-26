#include "customlineedit.h"

#include <QPainter>


CustomLineEdit::CustomLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
}

void CustomLineEdit::SetIcon(const QIcon &icon, const QSize &size)
{
    pixmap = icon.pixmap(size);
    if (icon.isNull())
        setTextMargins(1, 1, 1, 1);
    else {
        setTextMargins(size.width() + 6, 1, 1, 1);
    }
}

void CustomLineEdit::paintEvent(QPaintEvent * event)
{
    QLineEdit::paintEvent(event);
    if (!pixmap.isNull()) {
        QPainter painter(this);
        int y = (height() - pixmap.height()) / 2;
        painter.drawPixmap(4, y, pixmap);
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
