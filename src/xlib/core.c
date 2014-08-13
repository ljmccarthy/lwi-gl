#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <unistd.h>
#include "lwi_private.h"
#include "macros.h"

EXPORT lwi_time_t
lwi_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * LWI_SECOND + ts.tv_nsec;
}

EXPORT unsigned
lwi_screen_get_colour_depth(void)
{
    return DefaultDepth(X.dpy, X.screen_nr);
}

EXPORT Display *
lwi_x11_get_display(void)
{
    return X.dpy;
}

EXPORT int
lwi_x11_get_display_filedes(void)
{
    return ConnectionNumber(X.dpy);
}

EXPORT Window
lwi_x11_get_xid(lwi_window_t *w)
{
    return w->wid;
}

static void
lwi_canvas_init(lwi_canvas_t *d, Drawable did, Drawable did_front)
{
    d->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    d->width = 0;
    d->height = 0;
    d->clear_colour = 0;
    d->did = did;
    d->did_front = did_front;
    d->cid = XCreateGC(X.dpy, d->did, 0, NULL);
//    d->pid = 0;
#ifdef LWI_BUILD_CAIRO
    d->cairo = NULL;
#endif
}

static void
lwi_canvas_fini(lwi_canvas_t *d)
{
    pthread_mutex_destroy(&d->lock);
    XFreeGC(X.dpy, d->cid);
#ifdef LWI_BUILD_CAIRO
    if (d->cairo)
        cairo_surface_destroy(d->cairo);
#endif
}

#define ATTR_MASK        \
    ( CWBackPixel    \
    | CWBorderPixel  \
    | CWBitGravity   \
    | CWBackingStore \
    | CWEventMask    \
    | CWColormap )

static void
init_window(lwi_window_t *w, Window wid, unsigned sx, unsigned sy)
{
    memset(w, 0, sizeof(lwi_window_t));
    w->wid = wid;
    w->flags = WF_FRAMED;
    w->x = -0x8000;
    w->y = -0x8000;
    w->width = sx;
    w->height = sy;

    /* initialize canvas */
    Drawable did = w->wid;
    Drawable did_front = w->wid;
    if (X_HAS_DBE)
        did = XdbeAllocateBackBufferName(X.dpy, w->wid, XdbeBackground);
    lwi_canvas_init(&w->canvas, did, did_front);

    /* size hints for wm */
    w->size_hints.flags |= PSize;
    w->size_hints.width = sx;
    w->size_hints.height = sy;
    XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);

    /* tell window manager which events we can handle */
    Atom wm_protocols[] = {
        X_ATOM(WM_DELETE_WINDOW),
        //X_ATOM(WM_SAVE_YOURSELF),
        X_ATOM(WM_TAKE_FOCUS),
        X_ATOM(_NET_WM_PING),
        X_ATOM(_NET_WM_SYNC_REQUEST),
    };
    XSetWMProtocols(X.dpy, w->wid, wm_protocols, NELEMS(wm_protocols));

    /* wm protocol for killing hung process */
    XSetWMProperties(X.dpy, wid, NULL, NULL, NULL, 0, NULL, NULL, NULL);
    pid_t pid = getpid();
    XChangeProperty(
        X.dpy, wid, X_ATOM(_NET_WM_PID), XA_CARDINAL, 32,
        PropModeReplace, (void *) &pid, 1);
//    XChangeProperty(
//        X.dpy, wid, X_ATOM(WM_CLIENT_LEADER), XA_WINDOW, 32,
//        PropModeReplace, (void *) &wid, 1);

    if (X_HAS_SYNC) {
        w->netwm_sync_value.hi = 0;
        w->netwm_sync_value.lo = 0;
        w->netwm_sync_counter = XSyncCreateCounter(X.dpy, w->netwm_sync_value);
        XChangeProperty(
            X.dpy, wid, X_ATOM(_NET_WM_SYNC_REQUEST_COUNTER), XA_CARDINAL, 32,
            PropModeReplace, (void *) &w->netwm_sync_counter, 1);
    }

    XSaveContext(X.dpy, w->wid, X.context, (XPointer) w);
    LWI.window_count++;
}

static lwi_window_t *
create_window(unsigned sx, unsigned sy, Visual *visual, XSetWindowAttributes *attrs)
{
    if (sx == 0 || sy == 0)
        return NULL;

    lwi_window_t *w = malloc(sizeof(lwi_window_t));
    if (!w)
        return NULL;

    X_LOCK();
    Window wid = XCreateWindow(
        X.dpy, DefaultRootWindow(X.dpy),
        0, 0, sx, sy, 0, DefaultDepth(X.dpy, X.screen_nr),
        InputOutput, visual, ATTR_MASK, attrs);
    init_window(w, wid, sx, sy);
    X_UNLOCK();

    return w;
}

/*
need colormap argument?

lwi_window_t *
lwi_x11_from_xid(Window xid)
{
    lwi_window_t *w = malloc(sizeof(lwi_window_t));
    if (!w)
        return NULL;

    X_LOCK();
    init_window(w, xid);
    X_UNLOCK();
    return w;
}
*/

EXPORT lwi_window_t *
lwi_open(unsigned sx, unsigned sy)
{
    X_LOCK();
    lwi_window_t *w = create_window(sx, sy, X.visual, &X.win_attrs);
    X_UNLOCK();
    return w;
}

static XVisualInfo *
get_gl_visual(unsigned flags)
{
    int attribs[15] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_ALPHA_SIZE, 1,
        None, None,
        None, None,
        None
    };
    unsigned i = 10;
    if (flags & LWI_GL_DEPTH) {
        attribs[i++] = GLX_DEPTH_SIZE;
        attribs[i++] = 1;
    }
    if (flags & LWI_GL_STENCIL) {
        attribs[i++] = GLX_STENCIL_SIZE;
        attribs[i++] = 1;
    }
    XVisualInfo *vinfo = glXChooseVisual(X.dpy, X.screen_nr, attribs);
    return vinfo;
}

EXPORT lwi_window_t *
lwi_gl_open(unsigned sx, unsigned sy, unsigned flags)
{
    if (!X_HAS_GL)
        return NULL;

    X_LOCK();

    XVisualInfo *vinfo = get_gl_visual(flags);
    if (!vinfo)
        return NULL;

    XSetWindowAttributes attrs;
    memcpy(&attrs, &X.win_attrs, sizeof(XSetWindowAttributes));
    attrs.colormap = XCreateColormap(X.dpy, DefaultRootWindow(X.dpy), vinfo->visual, AllocNone);

    lwi_window_t *w = create_window(sx, sy, vinfo->visual, &attrs);
    if (!w) {
        XFreeColormap(X.dpy, attrs.colormap);
        X_UNLOCK();
        return NULL;
    }

    X_UNLOCK();

    w->glx_vinfo = vinfo;
    w->glx_colormap = attrs.colormap;

    return w;
}

/*
  Note: Destroying the Window will deallocate its DBE XdbeBackBuffer.
*/
EXPORT void
lwi_close(lwi_window_t *w)
{
    X_LOCK();
    if (w->wid) {
        if (w == X.mode_window)
            lwi_screen_restore_mode();
        if (w->glx_cid) {
            if (glXGetCurrentContext() == w->glx_cid) {
                glXWaitGL();
                glXMakeCurrent(X.dpy, None, NULL);
            }
            if (w->glx_colormap)
                XFreeColormap(X.dpy, w->glx_colormap);
            glXDestroyContext(X.dpy, w->glx_cid);
            w->glx_cid = 0;
            w->glx_colormap = 0;
        }
        lwi_canvas_fini(&w->canvas);
        XDestroyWindow(X.dpy, w->wid);
        w->wid = None;
    }
    X_UNLOCK();
}

static void
destroy_notify(lwi_window_t *w, Window wid)
{
    NOTICE("Window @ %p (XID %ld) destroyed", w, wid);

    XDeleteContext(X.dpy, wid, X.context);
    LWI.window_count--;

    memset(w, 0, sizeof(lwi_window_t));
    free(w);
}

EXPORT void
lwi_drop_target(lwi_window_t *w, int val)
{
    X_LOCK();
    if (val) {
        /* advertise that we support XDND version 4 */
        int dnd_version = 4;
        XChangeProperty(
            X.dpy, w->wid, X_ATOM(XdndAware), XA_ATOM, 32,
            PropModeReplace, (void *) &dnd_version, 1);
    }
    else {
        XDeleteProperty(X.dpy, w->wid, X_ATOM(XdndAware));
    }
    X_UNLOCK();
}

static int
xdnd_type_bit(Atom type)
{
    if (type == XA_STRING ||
        type == X_ATOM(UTF8_STRING) ||
        type == X_MIME_TYPE(text, plain) ||
        type == X_MIME_TYPE(text, unicode))
        return 1 << LWI_DND_STRING;

    if (type == X_MIME_TYPE(text, x_moz_url) ||
        type == X_MIME_TYPE(text, uri_list))
        return 1 << LWI_DND_URI;

    return 0;
}

/*
  Array elements must be freed with XFree
  Array must be freed with free
*/
int
get_xdnd_types(XClientMessageEvent *xe)
{
    int types = 0;

    Window src = xe->data.l[0];
    types |= xdnd_type_bit(xe->data.l[2]);
    types |= xdnd_type_bit(xe->data.l[3]);
    types |= xdnd_type_bit(xe->data.l[4]);

    if (xe->data.l[1] & 1) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_after;
        Atom *typelist;

        int ret = XGetWindowProperty(
            X.dpy, src, X_ATOM(XdndTypeList), 0, 0xFFFFFFFF,
            False, XA_ATOM, &actual_type, &actual_format, &nitems,
            &bytes_after, (unsigned char **) &typelist);
        if (ret != Success)
            return 0;

        for (unsigned long i = 0; i < nitems; i++) {
            types |= xdnd_type_bit(typelist[i]);

            char *name = XGetAtomName(X.dpy, typelist[i]);
            printf("  %s\n", name);
            XFree(name);
        }
        XFree(typelist);
    }
    return types;
}

static int
translate_xdnd_enter(XClientMessageEvent *xe)
{
    printf("XdndEnter %08x\n", get_xdnd_types(xe));
    return 0;
}

static int
translate_wm_protocols(XClientMessageEvent *xe, lwi_event_t *evt, lwi_window_t *w)
{
    Atom type = xe->data.l[0];

    if (type == X_ATOM(WM_DELETE_WINDOW)) {
        evt->type = LWI_WINDOW_CLOSE;
        return 1;
    }
    else if (type == X_ATOM(WM_SAVE_YOURSELF)) {
        evt->type = LWI_SHUTDOWN;
        return 1;
    }
    else if (type == X_ATOM(_NET_WM_PING)) {
        xe->window = w->wid;
        XSendEvent(X.dpy, DefaultRootWindow(X.dpy), False,
            SubstructureNotifyMask | SubstructureRedirectMask, (XEvent *) xe);
        //printf("ping!\n");
    }
    else if (type == X_ATOM(_NET_WM_SYNC_REQUEST)) {
        if (w->flags & WF_NETWM_SYNC)
            XSyncSetCounter(X.dpy, w->netwm_sync_counter, w->netwm_sync_value);
        w->netwm_sync_value.hi = xe->data.l[3];
        w->netwm_sync_value.lo = xe->data.l[2];
        w->flags |= WF_NETWM_SYNC;
        printf("sync request\n");
    }
    return 0;
}

static int
translate_client(XClientMessageEvent *xe, lwi_event_t *evt, lwi_window_t *w)
{
    const Atom type = xe->message_type;

    if (type == X_ATOM(WM_PROTOCOLS)) {
        return translate_wm_protocols(xe, evt, w);
    }
#if 1
    else if (type == X_ATOM(XdndEnter)) {
        translate_xdnd_enter(xe);
        return 0;
    }
    else if (type == X_ATOM(XdndLeave)) {
        printf("XdndLeave\n");
        return 0;
    }
    else if (type == X_ATOM(XdndPosition)) {
        printf("XdndPosition\n");
        return 0;
    }
    else if (type == X_ATOM(XdndStatus)) {
        printf("XdndStatus\n");
        return 0;
    }
    else if (type == X_ATOM(XdndDrop)) {
        printf("XdndDrop\n");
        return 0;
    }
    else if (type == X_ATOM(XdndFinished)) {
        printf("XdndFinished\n");
        return 0;
    }
#endif
    else {
        NOTICE("Unhandled client event %d", (int) type);
        return 0;
    }
}

static int
translate_expose(XExposeEvent *xe, lwi_event_t *evt)
{
    evt->type = LWI_WINDOW_EXPOSE;
    evt->x = xe->x;
    evt->y = xe->y;
    evt->width = xe->width;
    evt->height = xe->height;
    return 1;
}

static int
translate_configure(XConfigureEvent *xe,
                    lwi_event_t *evt, lwi_window_t *w)
{
    /* set netwm sync counter */
    if (w->flags & WF_NETWM_SYNC) {
        XSyncSetCounter(X.dpy, w->netwm_sync_counter, w->netwm_sync_value);
        w->flags &= ~WF_NETWM_SYNC;
        printf("sync done\n");
    }

    if (xe->width != w->width || xe->height != w->height) {
        evt->type = LWI_WINDOW_RESIZE;
        evt->width = w->width = xe->width;
        evt->height = w->height = xe->height;
        return 1;
    }
    else if (xe->x != w->x || xe->y != w->y) {
        evt->type = LWI_WINDOW_MOVE;
        evt->x = w->x = xe->x;
        evt->y = w->y = xe->y;
        return 1;
    }
    return 0;
}

static int
translate_visibility(XVisibilityEvent *xe,
                     lwi_event_t *evt)
{
    if (xe->state == VisibilityFullyObscured)
        evt->type = LWI_WINDOW_HIDE;
    else
        evt->type = LWI_WINDOW_SHOW;
    return 1;
}

static void
mapping_notify(XMappingEvent *xe)
{
    if (xe->request == MappingKeyboard)
        XRefreshKeyboardMapping(xe);
}

static int
translate_with_window(XEvent *xe, lwi_event_t *evt, lwi_window_t *w)
{
    switch (xe->type) {
    case KeyPress:
        evt->type = LWI_KEY_DOWN;
        goto key;
    case KeyRelease:
        evt->type = LWI_KEY_UP;
        goto key;
    case ButtonPress:
        evt->type = LWI_BUTTON_DOWN;
        goto key;
    case ButtonRelease:
        evt->type = LWI_BUTTON_UP;
    key:
        evt->key = LWI_KEY(xe->xkey.keycode, xe->xkey.state);
    mousepos:
        evt->x = xe->xkey.x;
        evt->y = xe->xkey.y;
        evt->time = xe->xkey.time;
        return 1;
    case MotionNotify:
        evt->type = LWI_MOUSE_MOVE;
        goto mousepos;
    case EnterNotify:
        evt->type = LWI_MOUSE_ENTER;
        goto mousepos;
    case LeaveNotify:
        evt->type = LWI_MOUSE_LEAVE;
        goto mousepos;
    case FocusIn:
        evt->type = LWI_WINDOW_FOCUS_IN;
        return 1;
    case FocusOut:
        evt->type = LWI_WINDOW_FOCUS_OUT;
        return 1;
    case Expose:
        return translate_expose(&xe->xexpose, evt);
    case VisibilityNotify:
        return translate_visibility(&xe->xvisibility, evt);
    case DestroyNotify:
        destroy_notify(w, xe->xdestroywindow.event);
        return 0;
    case ConfigureNotify:
        return translate_configure(&xe->xconfigure, evt, w);
    case ClientMessage:
        return translate_client(&xe->xclient, evt, w);
    default:
        return 0;
    }
}

static int
translate(XEvent *xe, lwi_event_t *evt)
{
    XPointer ptr;
    lwi_window_t *w = NULL;

    memset(evt, 0, sizeof(*evt));

    switch (xe->type) {
    case MappingNotify:
        mapping_notify(&xe->xmapping);
        return 0;
    default:
        if (XFindContext(X.dpy, xe->xany.window, X.context, &ptr) != 0)
            return 0;
        w = (lwi_window_t *) ptr;
        if (w->wid == None && xe->type != DestroyNotify)
            return 0;
        if (translate_with_window(xe, evt, w)) {
            evt->window = w;
            return 1;
        }
        return 0;
    }
}

EXPORT int
lwi_poll(lwi_event_t *evt)
{
    XEvent xe, next_xe;
    int rc = 0;

    X_LOCK();

    if (!XEventsQueued(X.dpy, QueuedAfterFlush))
        goto out;

    XNextEvent(X.dpy, &xe);
    if (!translate(&xe, evt))
        goto out;

    /* check for key repeat */
    if (xe.type == KeyPress && XEventsQueued(X.dpy, QueuedAlready)) {
        XPeekEvent(X.dpy, &next_xe);
        if (next_xe.type == KeyRelease
         && xe.xkey.window == next_xe.xkey.window
         && xe.xkey.keycode == next_xe.xkey.keycode
         && (next_xe.xkey.time - xe.xkey.time) < 2) {
            XNextEvent(X.dpy, &next_xe);
            evt->type = LWI_KEY_REPEAT;
        }
    }
    rc = 1;
out:
    X_UNLOCK();
    return rc;
}

static int
events_queued(void)
{
    X_LOCK();
    int queued = XEventsQueued(X.dpy, QueuedAfterFlush);
    X_UNLOCK();
    return queued;
}

/* poll X socket descriptor */
static int
poll_display(struct timespec *ts)
{
    struct pollfd fds[1] = {{ConnectionNumber(X.dpy), POLLIN, 0}};
    return ppoll(fds, 1, ts, NULL);
}

EXPORT int
lwi_wait(void)
{
    if (!LWI.window_count)
        return 0;
    while (!events_queued())
        poll_display(NULL);
    return 1;
}

#define MIN_WAIT (30 * LWI_MICRO_SECOND)

EXPORT int
lwi_wait_until(lwi_time_t timeout)
{
    if (timeout <= 0)
        return events_queued() != 0;

    /* no windows -> no events! */
    if (!LWI.window_count)
        return 0;

    /* absolute time of timeout duration */
    lwi_time_t tot = lwi_time() + timeout;

    while (1) {
        if (events_queued())
            return 1;

        /* calculate remaining time */
        lwi_time_t dt = tot - lwi_time();
        if (dt < MIN_WAIT)
            return 0;

        /* wait for event or timeout */
        struct timespec ts;
        ts.tv_sec = dt / LWI_SECOND;
        ts.tv_nsec = dt % LWI_SECOND;
        poll_display(&ts);

        /* check timeout */
        if (tot - lwi_time() < MIN_WAIT)
            return 0;
    }
}
