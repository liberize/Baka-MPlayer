#ifndef MPVWIDGET_H
#define MPVWIDGET_H

class QWidget;
class MpvHandler;

class MpvWidget {
public:
    virtual ~MpvWidget() {}
    virtual QWidget *self() = 0;
    virtual void setMpvHandler(MpvHandler *handler) = 0;
};

#endif // MPVWIDGET_H
