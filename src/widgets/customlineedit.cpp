#include "customlineedit.h"

#include <QPainter>

namespace {
    const int PADDING_LEFT_RIGHT = 3;
}

CustomLineEdit::CustomLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    setMouseTracking(true);
}

int CustomLineEdit::iconWidth() const
{
    return pixmap.isNull() ? 0 : pixmap.width() + PADDING_LEFT_RIGHT * 2;
}

void CustomLineEdit::SetIcon(const QIcon &icon, const QSize &size)
{
    pixmap = icon.pixmap(size);
    setTextMargins(iconWidth(), 1, 1, 1);
}

void CustomLineEdit::paintEvent(QPaintEvent * event)
{
    QLineEdit::paintEvent(event);
    if (!pixmap.isNull()) {
        QPainter painter(this);
        int x = (iconWidth() - pixmap.width()) / 2;
        int y = (height() - pixmap.height()) / 2;
        painter.drawPixmap(x, y, pixmap);
    }
}

void CustomLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return) { // do we need to check Qt::Key_Enter too?
        emit submitted(text());
        event->accept();
    } else
        QLineEdit::keyPressEvent(event);
}

void CustomLineEdit::mouseMoveEvent(QMouseEvent *event)
{
    QRect rect(0, 0, iconWidth(), height());
    setCursor(rect.contains(event->pos()) ? Qt::ArrowCursor : Qt::IBeamCursor);
    QLineEdit::mouseMoveEvent(event);
}
