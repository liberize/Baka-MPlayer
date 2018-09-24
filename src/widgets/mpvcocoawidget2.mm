#include "mpvcocoawidget.h"
#include "mpvhandler.h"

#include <QDebug>

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <AppKit/AppKit.h>
#import <OpenGL/gl.h>
#import <OpenGL/gl3.h>


@interface VideoView : NSOpenGLView

@property (nonatomic, assign) MpvHandler *mpvHandler;
@property (nonatomic, assign) mpv_render_context *mpvRenderContext;
@property (nonatomic, assign) QImage image;
@property (nonatomic, assign) GLuint framebuffer;
@property (nonatomic, assign) GLuint texture;

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

void mpvUpdateCallback(void *ctx)
{
    VideoView *view = (__bridge VideoView *)ctx;
    view.needsDisplay = YES;
}


@implementation VideoView

- (id)initWithFrame:(NSRect)frame {
    if (self = [super initWithFrame:frame]) {
        self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        
        _mpvHandler = NULL;
        _mpvRenderContext = NULL;
        _framebuffer = 0;
        _texture = 0;
    }
    return self;
}

- (void)dealloc {
    [self destroyGL];
    [self destroyMpv];
}

- (BOOL)isOpaque {
    return YES;
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

- (void)destroyMpv {
    if (_mpvRenderContext) {
        _mpvHandler->destroyRenderContext(_mpvRenderContext);
        _mpvRenderContext = NULL;
    }
}

- (void)setImage:(QImage)img {
    _image = img.mirrored(false, true).convertToFormat(QImage::Format_RGB888);
    self.needsDisplay = YES;
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

+ (NSOpenGLPixelFormat*)defaultPixelFormat {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAAllowOfflineRenderers,
        0
    };
    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
}

- (void)prepareOpenGL {
    GLint i = 1;
    [[self openGLContext] setValues:&i forParameter:NSOpenGLCPSwapInterval];
    
    glClearColor(0, 0, 0, 1);
    glClear(GLbitfield(GL_COLOR_BUFFER_BIT));
    [self initMpv];
}

-(void)drawRect:(NSRect)bounds {
    [[self openGLContext] makeCurrentContext];
    
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
    }

    [[self openGLContext] flushBuffer];

    if (_mpvRenderContext && _image.isNull())
        mpv_render_context_report_swap(_mpvRenderContext);
}

- (void) reshape {
    [[self openGLContext] makeCurrentContext];
    NSRect bounds = [self bounds];
    glViewport(NSMinX(bounds), NSMinY(bounds), NSWidth(bounds), NSHeight(bounds));
}

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
        if (view)
            view.mpvHandler = handler;
    }
}

void MpvCocoaWidget::setContentImage(const QImage &img)
{
    @autoreleasepool {
        VideoView *view = (VideoView *)cocoaView();
        if (view)
            [view setImage:img];
    }
}
