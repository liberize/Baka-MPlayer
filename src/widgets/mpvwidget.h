#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QImage>

class QWidget;
class MpvHandler;

class MpvWidget {
public:
    virtual ~MpvWidget() {}
    virtual QWidget *self() = 0;
    virtual void setMpvHandler(MpvHandler *handler) = 0;
    virtual void setContentImage(const QImage &img) = 0;
};

#endif // MPVWIDGET_H
