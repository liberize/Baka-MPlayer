#include "mpvcocoawidget.h"
#include "mpvhandler.h"

#include <QOpenGLTexture>
#include <QDebug>

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <AppKit/AppKit.h>
#import <OpenGL/gl.h>
#import <OpenGL/gl3.h>


@interface ViewLayer : CAOpenGLLayer

@property (nonatomic, strong) dispatch_queue_t mpvGLQueue;
@property (nonatomic, assign) MpvHandler *mpvHandler;
@property (nonatomic, assign) mpv_render_context *mpvRenderContext;
@property (nonatomic, strong) NSLock *uninitLock;
@property (nonatomic, assign) QImage image;
@property (nonatomic, assign) GLuint framebuffer;
@property (nonatomic, assign) GLuint texture;

@end

@interface VideoView : NSView
@end

static void *mpvGetOpenGL(void *ctx, const char* name)
{
    Q_UNUSED(ctx);
    CFBundleRef framework = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
    if (framework == NULL) {
        NSLog(@"Cannot get OpenGL function pointer!");
        return NULL;
    }
    CFStringRef symbolName = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingASCII);
    void *symbol = (void *)CFBundleGetFunctionPointerForName(framework, symbolName);
    CFRelease(symbolName);
    return symbol;
}

void mpvUpdateCallback(void *ctx) {
    ViewLayer *layer = (__bridge ViewLayer *)ctx;
    dispatch_async(layer.mpvGLQueue, ^{
        [layer display];
    });
}

@implementation ViewLayer

- (id)init
{
    if (self = [super init])
    {
        dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INTERACTIVE, -1);
        _mpvGLQueue = dispatch_queue_create("com.baka.mpvgl", attr);
        self.opaque = YES;
        self.asynchronous = NO;
        self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

        _mpvHandler = NULL;
        _mpvRenderContext = NULL;
        _uninitLock = [[NSLock alloc] init];
        _framebuffer = 0;
        _texture = 0;
    }
    return self;
}

- (void)dealloc {
    [_uninitLock lock];
    [self destroyGL];
    if (_mpvRenderContext) {
        _mpvHandler->destroyRenderContext(_mpvRenderContext);
        _mpvRenderContext = NULL;
    }
    [_uninitLock unlock];
}

- (void)initMpv {
    mpv_opengl_init_params glInit { &mpvGetOpenGL, nullptr, nullptr };
    mpv_render_param params[] {
        { MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, (void*)&glInit },
        { MPV_RENDER_PARAM_INVALID, nullptr },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    _mpvRenderContext = _mpvHandler->createRenderContext(params);
    mpv_render_context_set_update_callback(_mpvRenderContext, mpvUpdateCallback, (__bridge void *)self);
}

- (void)setImage:(QImage)img {
    _image = img.mirrored(false, true).convertToFormat(QImage::Format_RGB888);
    [self setNeedsDisplay];
}

- (void)initGL {
    glGenFramebuffers(1, &_framebuffer);

    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, _image.width(), _image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, (const GLvoid *)_image.bits());
    glBindTexture(GL_TEXTURE_2D, 0);
}
- (void)destroyGL {
    if (_framebuffer) {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    if (_texture) {
        glDeleteTextures(1, &_texture);
        _texture = 0;
    }
}

- (void)ignoreGLError {
    glGetError();
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask
{
    Q_UNUSED(mask);
    CGLPixelFormatAttribute attributes0[] = {
        // kCGLPFADisplayMask, (CGLPixelFormatAttribute)mask,
        kCGLPFADoubleBuffer,
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAAccelerated,
        kCGLPFAAllowOfflineRenderers,
        (CGLPixelFormatAttribute)0
    };
    CGLPixelFormatAttribute attributes1[] = {
        // kCGLPFADisplayMask, (CGLPixelFormatAttribute)mask,
        kCGLPFADoubleBuffer,
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAAllowOfflineRenderers,
        (CGLPixelFormatAttribute)0
    };
    CGLPixelFormatAttribute attributes2[] = {
        // kCGLPFADisplayMask, (CGLPixelFormatAttribute)mask,
        kCGLPFADoubleBuffer,
        kCGLPFAAllowOfflineRenderers,
        (CGLPixelFormatAttribute)0
    };

    CGLPixelFormatObj pixFormatObj = NULL;
    GLint numPixFormats = 0;

    CGLChoosePixelFormat(attributes0, &pixFormatObj, &numPixFormats);
    if (pixFormatObj == NULL) {
        CGLChoosePixelFormat(attributes1, &pixFormatObj, &numPixFormats);
    }
    if (pixFormatObj == NULL) {
        CGLChoosePixelFormat(attributes2, &pixFormatObj, &numPixFormats);
    }
    if (pixFormatObj == NULL) {
        NSLog(@"Cannot create OpenGL pixel format!");
    }
    return pixFormatObj;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat
{
    CGLContextObj context = [super copyCGLContextForPixelFormat:pixelFormat];
    GLint i = 1;
    CGLSetParameter(context, kCGLCPSwapInterval, &i);
    CGLEnable(context, kCGLCEMPEngine);
    CGLSetCurrentContext(context);
    return context;
}

- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext pixelFormat:(CGLPixelFormatObj)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
{
    Q_UNUSED(glContext);
    Q_UNUSED(pixelFormat);
    Q_UNUSED(timeInterval);
    Q_UNUSED(timeStamp);
    return YES;
}

- (void)drawInCGLContext:(CGLContextObj)glContext pixelFormat:(CGLPixelFormatObj)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
{
    Q_UNUSED(pixelFormat);
    Q_UNUSED(timeInterval);
    Q_UNUSED(timeStamp);

    [_uninitLock lock];
    CGLLockContext(glContext);
    CGLSetCurrentContext(glContext);

    glClear(GLbitfield(GL_COLOR_BUFFER_BIT));

    GLint dims[] = {0, 0, 0, 0};
    glGetIntegerv(GLenum(GL_VIEWPORT), dims);

    if (_mpvRenderContext) {
        if (_image.isNull()) {
            [self destroyGL];

            GLint i = 0;
            glGetIntegerv(GLenum(GL_DRAW_FRAMEBUFFER_BINDING), &i);

            bool yes = true;
            mpv_opengl_fbo fbo { i, dims[2], dims[3], 0 };
            mpv_render_param params[] {
                { MPV_RENDER_PARAM_OPENGL_FBO, (void*)&fbo },
                { MPV_RENDER_PARAM_FLIP_Y, &yes }
            };
            mpv_render_context_render(_mpvRenderContext, params);
            [self ignoreGLError];
        } else {
            if (!_texture)
                [self initGL];

            glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebuffer);
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, _texture, 0);
            glBlitFramebuffer(0, 0, _image.width(), _image.height(),
                              0, 0, dims[2], dims[3],
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }
    } else {
        glClearColor(0, 0, 0, 1);
        glClear(GLbitfield(GL_COLOR_BUFFER_BIT));
        [self initMpv];
    }
    glFlush();

    CGLUnlockContext(glContext);
    [_uninitLock unlock];

    if (_mpvRenderContext && _image.isNull())
        mpv_render_context_report_swap(_mpvRenderContext);
}

- (void)draw {
    [self display];
}

- (void)display {
    [super display];
    [CATransaction flush];
}

@end

@implementation VideoView

- (id)initWithFrame:(NSRect)frame {
    if (self = [super initWithFrame:frame]) {
        // set up layer
        ViewLayer *layer = [[ViewLayer alloc] init];
        layer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
        self.layer = layer;
        self.wantsLayer = YES;

        // other settings
        self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        self.wantsBestResolutionOpenGLSurface = YES;
    }
    return self;
}

//- (BOOL)mouseDownCanMoveWindow {
//    return YES;
//}

- (BOOL)isOpaque {
    return YES;
}

- (void)draw:(NSRect)dirtyRect {
    Q_UNUSED(dirtyRect);
}

//- (BOOL)acceptsFirstMouse:(NSEvent *)event {
//    return YES;
//}

@end

MpvCocoaWidget::MpvCocoaWidget(QWidget *parent) :
    QMacCocoaViewContainer(0, parent)
{
    @autoreleasepool {
        VideoView *view = [[VideoView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
        setCocoaView(view);
    }
}

MpvCocoaWidget::~MpvCocoaWidget()
{
    @autoreleasepool {
        setCocoaView(nullptr);
    }
}

QWidget *MpvCocoaWidget::self()
{
    return this;
}

void MpvCocoaWidget::setMpvHandler(MpvHandler *handler)
{
    @autoreleasepool {
        VideoView *view = (VideoView *)cocoaView();
        if (view) {
            ViewLayer *layer = (ViewLayer *)view.layer;
            layer.mpvHandler = handler;
        }
    }
}

void MpvCocoaWidget::setContentImage(const QImage &img)
{
    @autoreleasepool {
        VideoView *view = (VideoView *)cocoaView();
        if (view) {
            ViewLayer *layer = (ViewLayer *)view.layer;
            [layer setImage:img];
        }
    }
}
