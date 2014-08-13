#ifndef LWI_PRIVATE_XLIB_H
#define LWI_PRIVATE_XLIB_H

#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/sync.h>
#include <GL/glx.h>

#ifdef LWI_BUILD_CAIRO
    #include <cairo-xlib.h>
    #define LWI_CAIRO
#endif

/* Global context */
#define X __lwi_context
#define LWI __lwi_context.lwi_context

#define LWI_X11
#include "lwi.h"
#include "../lwi_private.h"

#define EXPORT __attribute__((visibility("default")))

typedef struct lwi_context_xlib_t lwi_context_xlib_t;

struct lwi_context_xlib_t {
    lwi_context_t lwi_context;
    /* Xlib specific */
    Display *dpy;
    unsigned screen_nr;
    XErrorEvent error;
    XContext context;
    Visual *visual;
    XRenderPictFormat *pictformat;
    XSetWindowAttributes win_attrs;
    int num_modes;
    XF86VidModeModeInfo **modes;
    XF86VidModeModeInfo *original_mode;
    lwi_window_t *mode_window;
    unsigned extensions;    /* bitmap of supported extensions */
    int (*glXGetVideoSyncSGI)(unsigned int *);
    int (*glXWaitVideoSyncSGI)(int, int, unsigned int *);
    int (*glXSwapIntervalSGI)(int);
    int (*glXSwapIntervalMESA)(int);
    Bool (*glXGetSyncValuesOML)(Display* dpy, GLXDrawable drawable, int64_t *ust, int64_t *msc, int64_t *sbc);
    int64_t (*glXSwapBuffersMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder);
};

struct lwi_canvas_t {
    pthread_mutex_t lock;
    unsigned width, height;
    unsigned clear_colour;
    Drawable did, did_front;
    GC cid;
//    Picture pid;
#ifdef LWI_BUILD_CAIRO
    cairo_surface_t *cairo;
#endif
};

/* window flags */
static const int WF_MAPPED      = 0x1;
static const int WF_MAXIMISED   = 0x2;  // TODO
static const int WF_MINIMISED   = 0x4;  // TODO
static const int WF_FRAMED      = 0x8;
static const int WF_FULL_SCREEN = 0x10;
static const int WF_NETWM_SYNC  = 0x20;  // set when _NET_WM_SYNC_REQUEST received
static const int WF_VSYNC_SET   = 0x40;

struct lwi_window_t {
    Window wid;
    int flags;
    XSizeHints size_hints;
    short x, y;
    unsigned short width, height;
    lwi_canvas_t canvas;
    XVisualInfo *glx_vinfo;
    GLXContext glx_cid;
    Colormap glx_colormap;
    XSyncCounter netwm_sync_counter;
    XSyncValue netwm_sync_value;
    void *userdata;
};

struct lwi_server_image_t {
    Pixmap pid;
    unsigned width, height, depth;
};

/* use XSHM if shm.shmaddr != NULL */
struct lwi_client_image_t {
    XImage *ximg;
    XShmSegmentInfo shm;
};

/* X atoms - must be synced with atom_names in core.c */
enum {
    _X_ATOM_INDEX_UTF8_STRING,
    _X_ATOM_INDEX_WM_PROTOCOLS,
    _X_ATOM_INDEX_WM_DELETE_WINDOW,
    _X_ATOM_INDEX_WM_SAVE_YOURSELF,
    _X_ATOM_INDEX_WM_TAKE_FOCUS,
    _X_ATOM_INDEX_WM_CLIENT_MACHINE,
    _X_ATOM_INDEX_WM_CLIENT_LEADER,
    _X_ATOM_INDEX__MOTIF_WM_HINTS,
    _X_ATOM_INDEX__NET_WM_NAME,
    _X_ATOM_INDEX__NET_WM_STATE,
    _X_ATOM_INDEX__NET_WM_STATE_HIDDEN,
    _X_ATOM_INDEX__NET_WM_STATE_FULLSCREEN,
    _X_ATOM_INDEX__NET_WM_PID,
    _X_ATOM_INDEX__NET_WM_PING,
    _X_ATOM_INDEX__NET_WM_SYNC_REQUEST,
    _X_ATOM_INDEX__NET_WM_SYNC_REQUEST_COUNTER,
    _X_ATOM_INDEX_XdndAware,
    _X_ATOM_INDEX_XdndEnter,
    _X_ATOM_INDEX_XdndLeave,
    _X_ATOM_INDEX_XdndPosition,
    _X_ATOM_INDEX_XdndStatus,
    _X_ATOM_INDEX_XdndDrop,
    _X_ATOM_INDEX_XdndFinished,
    _X_ATOM_INDEX_XdndTypeList,
    _X_ATOM_INDEX_MIME_TYPE_text__plain,
    _X_ATOM_INDEX_MIME_TYPE_text__unicode,
    _X_ATOM_INDEX_MIME_TYPE_text__x_moz_url,
    _X_ATOM_INDEX_MIME_TYPE_text__uri_list,
};

enum {
    _X_EXTENSION_DBE,
    _X_EXTENSION_SHM,
    _X_EXTENSION_GLX,
    _X_EXTENSION_RENDER,
    _X_EXTENSION_VIDMODE,
    _X_EXTENSION_FIXES,
    _X_EXTENSION_SYNC,
};

#define X_ATOM(name) __lwi_atoms[_X_ATOM_INDEX_##name]
#define X_MIME_TYPE(type, subtype) X_ATOM(MIME_TYPE_##type##__##subtype)
#define X_HAS_EXTENSION(n) (X.extensions & (1 << (n)))

/* X extensions */
#define X_HAS_DBE     X_HAS_EXTENSION(_X_EXTENSION_DBE)
#define X_HAS_SHM     X_HAS_EXTENSION(_X_EXTENSION_SHM)
#define X_HAS_GL      X_HAS_EXTENSION(_X_EXTENSION_GLX)
#define X_HAS_RENDER  X_HAS_EXTENSION(_X_EXTENSION_RENDER)
#define X_HAS_VIDMODE X_HAS_EXTENSION(_X_EXTENSION_VIDMODE)
#define X_HAS_FIXES   X_HAS_EXTENSION(_X_EXTENSION_FIXES)
#define X_HAS_SYNC    X_HAS_EXTENSION(_X_EXTENSION_SYNC)

extern lwi_context_xlib_t __lwi_context;
extern Atom __lwi_atoms[];

#define X_LOCK() do { XLockDisplay(X.dpy); } while (0)
#define X_UNLOCK() do { XUnlockDisplay(X.dpy); } while (0)

/* Private functions */

void
__lwi_enable_frame(lwi_window_t *w);

void
__lwi_disable_frame(lwi_window_t *w);

void
__lwi_enable_full_screen(lwi_window_t *w);

void
__lwi_disable_full_screen(lwi_window_t *w);

#endif /* LWI_PRIVATE_XLIB_H */
