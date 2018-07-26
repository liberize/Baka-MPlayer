#ifndef MPVGLWIDGET_H
#define MPVGLWIDGET_H

#include <QtWidgets/QOpenGLWidget>
#include <mpv/client.h>
#include <mpv/qthelper.hpp>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include "mpvwidget.h"


class MpvGlWidget : public QOpenGLWidget, public MpvWidget {
    Q_OBJECT

public:
    explicit MpvGlWidget(QWidget *parent = nullptr);
    ~MpvGlWidget();

protected:
    QWidget *self();
    void setMpvHandler(MpvHandler *handler);
    void setContentImage(const QImage &img);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    static void render_update(void *ctx);

private slots:
    void maybeUpdate();
    void onFrameSwapped();

private:
    MpvHandler *mpv = nullptr;
    mpv_render_context *render = nullptr;
    int glWidth = 0, glHeight = 0;
    QImage image;
};

#endif // MPVGLWIDGET_H
