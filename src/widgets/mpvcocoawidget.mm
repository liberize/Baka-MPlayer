#ifdef Q_OS_DARWIN

#include "mpvcocoawidget.h"
#include "mpvhandler.h"

#import <Foundation/Foundation.h>
#import <OpenGL/gl.h>
#import <OpenGL/gl3.h>

#pragma mark - Global Functions

static void *mpvGetOpenGL(void *ctx, const char* name)
{
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

#pragma mark - ViewLayer Class

@interface ViewLayer : CAOpenGLLayer

@property (nonatomic, weak) VideoView *videoView;
@property (nonatomic, strong) dispatch_queue_t mpvGLQueue;

@end

@implementation ViewLayer

- (id)init
{
    if (self = [super init])
    {
        dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INTERACTIVE, -1);
        _mpvGLQueue = dispatch_queue_create("com.colliderli.iina.mpvgl", attr);
        self.opaque = YES;
        self.asynchronous = NO;
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    }
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask
{
    CGLPixelFormatAttribute attributes0[] = {
        kCGLPFADoubleBuffer,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        kCGLPFAAccelerated,
        kCGLPFAAllowOfflineRenderers,
        0
    };
    CGLPixelFormatAttribute attributes1[] = {
        kCGLPFADoubleBuffer,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        kCGLPFAAllowOfflineRenderers,
        0
    };
    CGLPixelFormatAttribute attributes2[] = {
        kCGLPFADoubleBuffer,
        kCGLPFAAllowOfflineRenderers,
        0
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
    return YES;
}

- (void)drawInCGLContext:(CGLContextObj)glContext pixelFormat:(CGLPixelFormatObj)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
{
    [_videoView.uninitLock lock];

    if (!_videoView.mpvRenderContext) {
        [_videoView.uninitLock unlock];
        return;
    }

    CGLLockContext(glContext);
    CGLSetCurrentContext(glContext);

    glClear(GLbitfield(GL_COLOR_BUFFER_BIT));

    GLint i = 0;
    glGetIntegerv(GLenum(GL_DRAW_FRAMEBUFFER_BINDING), &i);
    GLint dims[] = {0, 0, 0, 0};
    glGetIntegerv(GLenum(GL_VIEWPORT), &dims);

    mpv_render_context *context = _videoView.mpvRenderContext;
    if (context) {
        bool yes = true;
        mpv_opengl_fbo fbo { i, dims[2], dims[3], 0 };
        mpv_render_param params[] {
            {MPV_RENDER_PARAM_OPENGL_FBO, (void*)&fbo },
            {MPV_RENDER_PARAM_FLIP_Y, &yes}
        };
        mpv_render_context_render(context, params);
        ignoreGLError();
    } else {
        glClearColor(0, 0, 0, 1);
        glClear(GLbitfield(GL_COLOR_BUFFER_BIT));
    }
    glFlush();

    CGLUnlockContext(glContext);
    [_videoView.uninitLock unlock];

    context = _videoView.mpvRenderContext;
    if (context) {
        mpv_render_context_report_swap(context);
    }
}

- (void)draw {
    [self display];
}

- (void)display {
    [super display];
    [CATransaction flush];
}

- (void)gle {
    GLenum e = glGetError();
    switch (e) {
        case GLenum(GL_NO_ERROR):
            break;
        case GLenum(GL_OUT_OF_MEMORY):
            NSLog(@"GL_OUT_OF_MEMORY");
            break;
        case GLenum(GL_INVALID_ENUM):
            NSLog(@"GL_INVALID_ENUM");
            break;
        case GLenum(GL_INVALID_VALUE):
            NSLog(@"GL_INVALID_VALUE");
            break;
        case GLenum(GL_INVALID_OPERATION):
            NSLog(@"GL_INVALID_OPERATION");
            break;
        case GLenum(GL_INVALID_FRAMEBUFFER_OPERATION):
            NSLog(@"GL_INVALID_FRAMEBUFFER_OPERATION");
            break;
        case GLenum(GL_STACK_UNDERFLOW):
            NSLog(@"GL_STACK_UNDERFLOW");
            break;
        case GLenum(GL_STACK_OVERFLOW):
            NSLog(@"GL_STACK_OVERFLOW");
            break;
        default:
            break
    }
}

- (void)ignoreGLError {
    glGetError();
}

@end

#pragma mark - VideoView Class

@interface VideoView : NSView

@property (nonatomic, strong) ViewLayer *videoLayer;
@property (nonatomic, assign) MpvHandler *mpvHandler;
@property (nonatomic, assign) mpv_render_context *mpvRenderContext;
@property (nonatomic, strong) NSLock *uninitLock;

@end

@implementation VideoView

- (id)initWithFrame:(CGRect)frame {
    [super initWithFrame:frame];

    // set up layer
    _videoLayer = [[ViewLayer alloc] init];
    _videoLayer.videoView = self;
    _videoLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
    self.layer = _videoLayer;
    self.wantsLayer = YES;

    // other settings
    self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.wantsBestResolutionOpenGLSurface = YES;

    _mpvHandler = NULL;
    _mpvRenderContext = NULL;
    _uninitLock = [[NSLock alloc] init];
}

- (void)initMpv:(MpvHandler *)mpv {
    [_uninitLock lock];
    if (_mpvRenderContext) {
        _mpvhandler->destroyRenderContext(_mpvRenderContext);
    }
    mpv_opengl_init_params glInit { &mpvGetOpenGL, nullptr, nullptr };
    mpv_render_param params[] {
        { MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, (void*)&glInit },
        { MPV_RENDER_PARAM_INVALID, nullptr },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    _mpvhandler = mpv;
    _mpvRenderContext = _mpvHandler->createRenderContext(params);
    mpv_render_context_set_update_callback(_mpvRenderContext, mpvUpdateCallback, (__bridge void *)_videoLayer);
    [_uninitLock unlock];
}

- (void)dealloc {
    [_uninitLock lock];
    if (_mpvRenderContext) {
        mpvHandler->destroyRenderContext(_mpvRenderContext);
        _mpvRenderContext = NULL;
    }
    [_uninitLock unlock];
}

- (BOOL)mouseDownCanMoveWindow {
    return YES;
}

- (BOOL)isOpaque {
    return YES;
}

- (void)draw:(NSRect)dirtyRect {
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

@end

#pragma mark - MpvCocoaWidget Class

MpvCocoaWidget::MpvCocoaWidget(QWidget *parent) :
    QMacCocoaViewContainer(0, parent)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    VideoView *view = [[VideoView alloc] initWithFrame:CGRectMake(0, 0, width(), height())];
    setCocoaView(view);
    [view release];
    [pool release];
}

MpvCocoaWidget::~MpvCocoaWidget()
{
}

QWidget *MpvCocoaWidget::self()
{
    return this;
}

void MpvCocoaWidget::setMpvHandler(MpvHandler *handler)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSView *view = cocoaView();
    if (view) {
        [view initMpv:handler];
    }
    [pool release];
}

#endif
