#ifndef MPVCOCOAWIDGET_H
#define MPVCOCOAWIDGET_H


#include <QMacCocoaViewContainer>
#include <mpv/client.h>
#include <mpv/qthelper.hpp>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include "mpvwidget.h"


class MpvCocoaWidget : public QMacCocoaViewContainer, public MpvWidget
{
    Q_OBJECT

public:
    explicit MpvCocoaWidget(QWidget *parent = nullptr);
    ~MpvCocoaWidget();

protected:
    QWidget *self();
    void setMpvHandler(MpvHandler *handler);
};

#endif // MPVCOCOAWIDGET_H
