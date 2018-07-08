#ifndef MPVGLWIDGET_H
#define MPVGLWIDGET_H

#include <QtWidgets/QOpenGLWidget>
#include <mpv/client.h>
#include <mpv/qthelper.hpp>
#include <mpv/render.h>
#include <mpv/render_gl.h>

class MpvHandler;

class MpvGlWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit MpvGlWidget(QWidget *parent = nullptr);
    ~MpvGlWidget();

    QWidget *self();
    void setMpvHandler(MpvHandler *handler);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    static void render_update(void *ctx);

private slots:
    void maybeUpdate();
    void self_frameSwapped();

private:
    MpvHandler *mpv = nullptr;
    mpv_render_context *render = nullptr;
    int glWidth = 0, glHeight = 0;
};

#endif // MPVGLWIDGET_H
