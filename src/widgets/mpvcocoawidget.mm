#include "mpvcocoawidget.h"
#include "mpvhandler.h"

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <AppKit/AppKit.h>
#import <OpenGL/gl.h>
#import <OpenGL/gl3.h>

@class VideoView;

@interface ViewLayer : CAOpenGLLayer

@property (nonatomic, weak) VideoView *videoView;
@property (nonatomic, strong) dispatch_queue_t mpvGLQueue;

@end

@interface VideoView : NSView

@property (nonatomic, strong) ViewLayer *videoLayer;
@property (nonatomic, assign) MpvHandler *mpvHandler;
@property (nonatomic, assign) mpv_render_context *mpvRenderContext;
@property (nonatomic, strong) NSLock *uninitLock;

@end

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

@implementation ViewLayer

- (id)init
{
    if (self = [super init])
    {
        dispatch_queue_attr_t attr = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INTERACTIVE, -1);
        _mpvGLQueue = dispatch_queue_create("com.colliderli.iina.mpvgl", attr);
        self.opaque = YES;
        self.asynchronous = NO;
        self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    }
    return self;
}

- (void)initMpvStuff {
    mpv_opengl_init_params glInit { &mpvGetOpenGL, nullptr, nullptr };
    mpv_render_param params[] {
        { MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, (void*)&glInit },
        { MPV_RENDER_PARAM_INVALID, nullptr },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    _videoView.mpvRenderContext = _videoView.mpvHandler->createRenderContext(params);
    mpv_render_context_set_update_callback(_videoView.mpvRenderContext, mpvUpdateCallback, (__bridge void *)self);
}

- (void)ignoreGLError {
    glGetError();
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask
{
    CGLPixelFormatAttribute attributes0[] = {
        kCGLPFADoubleBuffer,
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAAccelerated,
        kCGLPFAAllowOfflineRenderers,
        (CGLPixelFormatAttribute)0
    };
    CGLPixelFormatAttribute attributes1[] = {
        kCGLPFADoubleBuffer,
        kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAAllowOfflineRenderers,
        (CGLPixelFormatAttribute)0
    };
    CGLPixelFormatAttribute attributes2[] = {
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
    return YES;
}

- (void)drawInCGLContext:(CGLContextObj)glContext pixelFormat:(CGLPixelFormatObj)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
{
    [_videoView.uninitLock lock];

    if (!_videoView.mpvRenderContext) {
        [self initMpvStuff];
    }

    CGLLockContext(glContext);
    CGLSetCurrentContext(glContext);

    glClear(GLbitfield(GL_COLOR_BUFFER_BIT));

    GLint i = 0;
    glGetIntegerv(GLenum(GL_DRAW_FRAMEBUFFER_BINDING), &i);
    GLint dims[] = {0, 0, 0, 0};
    glGetIntegerv(GLenum(GL_VIEWPORT), dims);

    mpv_render_context *context = _videoView.mpvRenderContext;
    if (context) {
        NSLog(@"view width: %d, height: %d", self.frame.size.width, self.frame.size.height);
        NSLog(@"opengl width: %d, height: %d", dims[2], dims[3]);

        bool yes = true;
        mpv_opengl_fbo fbo { i, dims[2], dims[3], 0 };
        mpv_render_param params[] {
            {MPV_RENDER_PARAM_OPENGL_FBO, (void*)&fbo },
            {MPV_RENDER_PARAM_FLIP_Y, &yes}
        };
        mpv_render_context_render(context, params);
        [self ignoreGLError];
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
            break;
    }
}

@end

@implementation VideoView

- (id)initWithFrame:(NSRect)frame {
    if (self = [super initWithFrame:frame]) {
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
    return self;
}

- (void)setMpvHandle:(MpvHandler *)mpv {
    _mpvHandler = mpv;
}

- (void)dealloc {
    [_uninitLock lock];
    if (_mpvRenderContext) {
        _mpvHandler->destroyRenderContext(_mpvRenderContext);
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

MpvCocoaWidget::MpvCocoaWidget(QWidget *parent) :
    QMacCocoaViewContainer(0, parent)
{
    @autoreleasepool {
        NSLog(@"width: %d, height: %d", width(), height());
        VideoView *view = [[VideoView alloc] initWithFrame:NSMakeRect(0, 0, width(), height())];
        setCocoaView(view);
    }
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
    @autoreleasepool {
        VideoView *view = (VideoView *)cocoaView();
        if (view) {
            [view setMpvHandler:handler];
        }
    }
}
