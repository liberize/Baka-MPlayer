#include "mpvglwidget.h"
#include <QtGlobal>
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
#include <QtX11Extras/QX11Info>
#include <qpa/qplatformnativeinterface.h>
#endif
#include <QLayout>
#include <QMainWindow>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QMetaObject>
#include <QDebug>
#include <QPainter>

#include <cmath>
#include <stdexcept>
#include <mpv/qthelper.hpp>
#include "mpvhandler.h"

#ifdef Q_OS_WIN
    #ifndef GLAPIENTRY
    // On Windows, GLAPIENTRY may sometimes conveniently go missing
    #define GLAPIENTRY __stdcall
    #endif
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

static void* GLAPIENTRY glMPGetNativeDisplay(const char* name) {
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    if (!strcmp(name, "x11")) {
        return QX11Info::display();
    }
#elif defined(Q_OS_WIN)
    if (!strcmp(name, "IDirect3DDevice9")) {
        // Do something here ?
    }
#else
    Q_UNUSED(name);
#endif
    return nullptr;
}

static void *get_proc_address(void *ctx, const char *name) {
    (void)ctx;
    auto glctx = QOpenGLContext::currentContext();
    if (!strcmp(name, "glMPGetNativeDisplay"))
        return (void*)glMPGetNativeDisplay;
    void *res = glctx ? (void*)glctx->getProcAddress(QByteArray(name)) : nullptr;

#ifdef Q_OS_WIN32
    // QOpenGLContext::getProcAddress() in Qt 5.6 and below doesn't resolve all
    // core OpenGL functions, so fall back to Windows' GetProcAddress().
    if (!res) {
        HMODULE module = (HMODULE)QOpenGLContext::openGLModuleHandle();
        if (!module) {
            // QOpenGLContext::openGLModuleHandle() returns NULL when Qt isn't
            // using dynamic OpenGL. In this case, openGLModuleType() can be
            // used to determine which module to query.
            switch (QOpenGLContext::openGLModuleType()) {
            case QOpenGLContext::LibGL:
                module = GetModuleHandleW(L"opengl32.dll");
                break;
            case QOpenGLContext::LibGLES:
                module = GetModuleHandleW(L"libGLESv2.dll");
                break;
            }
        }
        if (module)
            res = (void*)GetProcAddress(module, name);
    }
#endif

    return res;
}

MpvGlWidget::MpvGlWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    connect(this, &QOpenGLWidget::frameSwapped,
            this, &MpvGlWidget::onFrameSwapped);

    setContextMenuPolicy(Qt::CustomContextMenu);
}

MpvGlWidget::~MpvGlWidget()
{
    makeCurrent();
    if (render) {
        mpv->destroyRenderContext(render);
        render = nullptr;
    }
    doneCurrent();
}

QWidget *MpvGlWidget::self()
{
    return this;
}

void MpvGlWidget::setMpvHandler(MpvHandler *handler)
{
    mpv = handler;
}

void MpvGlWidget::setContentImage(const QImage &img)
{
    image = img;
    update();
}

void MpvGlWidget::initializeGL()
{
    mpv_opengl_init_params glInit { &get_proc_address, this, nullptr };
    mpv_render_param params[] {
        { MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, (void*)&glInit },
        { MPV_RENDER_PARAM_INVALID, nullptr },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    QWidget *nativeParent = nativeParentWidget();
    if (nativeParent == nullptr) {
        qDebug() << "[glwidget] no native parent handle";
    }
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    else if (QGuiApplication::platformName().contains("xcb")) {
        qDebug() << "[glwidget] assigning x11 display";
        params[2].type = MPV_RENDER_PARAM_X11_DISPLAY;
        params[2].data = (void*)QX11Info::display();
    } else if (QGuiApplication::platformName().contains("wayland")) {
        qDebug() << "[glwidget] assigning wayland display";
        QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
        params[2].type = MPV_RENDER_PARAM_WL_DISPLAY;
        params[2].data = native->nativeResourceForWindow("display", nullptr);
    } else {
        qDebug() << "[glwidget] unknown display mode (eglfs et al)";
    }
#endif

    render = mpv->createRenderContext(params);
    mpv_render_context_set_update_callback(render, MpvGlWidget::render_update, (void *)this);
}

void MpvGlWidget::paintGL()
{
    if (image.isNull()) {
        bool yes = true;
        mpv_opengl_fbo fbo { (int)defaultFramebufferObject(), glWidth, glHeight, 0 };
        mpv_render_param params[] {
            {MPV_RENDER_PARAM_OPENGL_FBO, (void*)&fbo },
            {MPV_RENDER_PARAM_FLIP_Y, &yes}
        };
        mpv_render_context_render(render, params);
    } else {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(QRect(0, 0, width(), height()), image);
    }
}

void MpvGlWidget::resizeGL(int w, int h)
{
    qreal r = devicePixelRatio();
    glWidth = int(w * r);
    glHeight = int(h * r);
}

void MpvGlWidget::render_update(void *ctx)
{
    QMetaObject::invokeMethod(reinterpret_cast<MpvGlWidget*>(ctx), "maybeUpdate");
}

void MpvGlWidget::maybeUpdate()
{
    if (window()->isMinimized()) {
        makeCurrent();
        paintGL();
        context()->swapBuffers(context()->surface());
        onFrameSwapped();
        doneCurrent();
    } else {
        update();
    }
}

void MpvGlWidget::onFrameSwapped()
{
    if (render && image.isNull())
        mpv_render_context_report_swap(render);
}
