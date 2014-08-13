#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "lwi_private.h"
#include "macros.h"

#define glXGetProcAddress(str, ret) ((ret (*)()) glXGetProcAddress((GLubyte *)(str)))

/* must be synced with atom enum in lwi_private.h */
static const char *const atom_names[] = {
    "UTF8_STRING",
    "WM_PROTOCOLS",
    "WM_DELETE_WINDOW",
    "WM_SAVE_YOURSELF",
    "WM_TAKE_FOCUS",
    "WM_CLIENT_MACHINE",
    "WM_CLIENT_LEADER",
    "_MOTIF_WM_HINTS",
    "_NET_WM_NAME",
    "_NET_WM_STATE",
    "_NET_WM_STATE_HIDDEN",
    "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_PID",
    "_NET_WM_PING",
    "_NET_WM_SYNC_REQUEST",
    "_NET_WM_SYNC_REQUEST_COUNTER",
    "XdndAware",
    "XdndEnter",
    "XdndLeave",
    "XdndPosition",
    "XdndStatus",
    "XdndDrop",
    "XdndFinished",
    "XdndTypeList",
    "text/plain",
    "text/unicode",
    "text/x-moz-url",
    "text/uri-list",
};

static int initmode = 0;
lwi_context_xlib_t __lwi_context;
Atom __lwi_atoms[NELEMS(atom_names)];
int __lwi_debug = 0;
static pthread_t main_thread;

static int
error_handler(Display *dpy, XErrorEvent *event)
{
    char errstr[256];
    memcpy(&X.error, event, sizeof(X.error));
    XGetErrorText(dpy, event->error_code, errstr, sizeof(errstr));
    fprintf(stderr, "LWI: X11 Error: %s\n", errstr);
    return 0;
}

static unsigned
query_extensions(Display *dpy)
{
    unsigned exts = 0;
    int a, b;

    if (XdbeQueryExtension(dpy, &a, &b))
        exts |= 1 << _X_EXTENSION_DBE;

    if (XShmQueryExtension(dpy))
        exts |= 1 << _X_EXTENSION_SHM;

    if (glXQueryExtension(dpy, &a, &b))
        exts |= 1 << _X_EXTENSION_GLX;

    if (XRenderQueryExtension(dpy, &a, &b))
        exts |= 1 << _X_EXTENSION_RENDER;

    if (XF86VidModeQueryExtension(X.dpy, &a, &b))
        exts |= 1 << _X_EXTENSION_VIDMODE;

    if (XFixesQueryExtension(X.dpy, &a, &b))
        exts |= 1 << _X_EXTENSION_FIXES;

    if (XSyncQueryExtension(X.dpy, &a, &b)
     && XSyncInitialize(X.dpy, &a, &b))
        exts |= 1 << _X_EXTENSION_SYNC;

    return exts;
}

EXPORT void
lwi_shutdown(void)
{
    if (!initmode)
        return;
    initmode = 0;

    NOTICE("Shutdown");
    lwi_screen_restore_mode();
}

/*
  This may cause a segfault but it's better than not restoring the resolution!
*/
static void
signal_handler(int signum)
{
    if (pthread_self() == main_thread) {
        NOTICE("Received signal: %s", strsignal(signum));
        lwi_shutdown();
        exit(0);
    }
    //pthread_exit(0);
    pthread_kill(pthread_self(), SIGABRT);
}

static Display *
open_display(void)
{
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        const char *dpy_name = getenv("DISPLAY");
        if (!dpy_name || !dpy_name[0])
            NOTICE("Error: DISPLAY is not set, is the X server running?");
        else
            NOTICE("Error: Failed to connect to X display: %s", dpy_name);
        return 0;
    }
    return dpy;
}

EXPORT int
lwi_init(void)
{
    if (initmode)
        return 1;
    initmode = 1;

    if (getenv("LWI_DEBUG"))
        __lwi_debug = 1;

    NOTICE("Initializing");

    if (!XInitThreads()) {
        NOTICE("Error: XInitThreads() failed");
        return 0;
    }

    memset(&__lwi_context, 0, sizeof(__lwi_context));
    XSetErrorHandler(error_handler);

    X.dpy = open_display();
    if (!X.dpy)
        return 0;

    X.screen_nr = DefaultScreen(X.dpy);
    X.context = XUniqueContext();
    X.visual = DefaultVisual(X.dpy, X.screen_nr);
    X.extensions = query_extensions(X.dpy);

    for (unsigned i = 0; i < NELEMS(atom_names); i++)
        __lwi_atoms[i] = XInternAtom(X.dpy, atom_names[i], False);

    /* attributes for regular windows */
    X.win_attrs.background_pixel = BlackPixel(X.dpy, X.screen_nr);
    X.win_attrs.border_pixel = BlackPixel(X.dpy, X.screen_nr);
    X.win_attrs.bit_gravity = NorthWestGravity;
    X.win_attrs.backing_store = NotUseful;
    X.win_attrs.event_mask = KeyPressMask
                           | KeyReleaseMask
                           | ButtonPressMask
                           | ButtonReleaseMask
                           | EnterWindowMask
                           | LeaveWindowMask
                           | PointerMotionMask
                           | ButtonMotionMask
                           | KeymapStateMask
                           | ExposureMask
                           | VisibilityChangeMask
                           | StructureNotifyMask
                           | SubstructureNotifyMask
                           | FocusChangeMask;
    X.win_attrs.colormap = XCreateColormap(
        X.dpy, DefaultRootWindow(X.dpy), X.visual, AllocNone);

    if (X_HAS_RENDER) {
        X.pictformat = XRenderFindVisualFormat(X.dpy, X.visual);
        if (!X.pictformat)
            X.extensions &= ~(1 << _X_EXTENSION_RENDER);
    }
    else {
        NOTICE("Display does not support Render");
    }

    if (X_HAS_GL) {
        X.glXGetVideoSyncSGI = glXGetProcAddress("glXGetVideoSyncSGI", int);
        X.glXWaitVideoSyncSGI = glXGetProcAddress("glXWaitVideoSyncSGI", int);
        X.glXSwapIntervalSGI = glXGetProcAddress("glXSwapIntervalSGI", int);
        X.glXSwapIntervalMESA = glXGetProcAddress("glXSwapIntervalMESA", int);
        X.glXGetSyncValuesOML = glXGetProcAddress("glXGetSyncValuesOML", int);
        X.glXSwapBuffersMscOML = glXGetProcAddress("glXSwapBuffersMscOML", int64_t);

        if (X.glXSwapIntervalSGI)
            NOTICE("OpenGL vsync using glXSwapIntervalSGI");
        else if (X.glXSwapIntervalMESA)
            NOTICE("OpenGL vsync using glXSwapIntervalMESA");
        else if (X.glXGetVideoSyncSGI && X.glXWaitVideoSyncSGI)
            NOTICE("OpenGL vsync using glXWaitVideoSyncSGI");
        else if (X.glXGetSyncValuesOML && X.glXSwapBuffersMscOML)
            NOTICE("OpenGL vsync using glXSwapBuffersMscOML");
        else
            NOTICE("OpenGL vsync not available");
    }
    else {
        NOTICE("Display does not support OpenGL");
    }

    /* get video modes */
    if (X_HAS_VIDMODE)
        XF86VidModeGetAllModeLines(X.dpy, X.screen_nr, &X.num_modes, &X.modes);
    else
        NOTICE("Display does not support video mode switching");

    /* cleanup on exit */
    if (atexit(lwi_shutdown)) {
        NOTICE("atexit() failed");
        return 0;
    }

    /* setup signal handling */
    main_thread = pthread_self();
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    XSync(X.dpy, True);

    NOTICE("Initialization successful");

    return 1;
}
