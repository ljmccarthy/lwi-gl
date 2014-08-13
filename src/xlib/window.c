#include <string.h>
#include "lwi_private.h"
#include "macros.h"

EXPORT void
lwi_set_user_data(lwi_window_t *w, void *p)
{
    w->userdata = p;
}

EXPORT void *
lwi_get_user_data(lwi_window_t *w)
{
    return w->userdata;
}

EXPORT void
lwi_visible(lwi_window_t *w, int val)
{
    if (val && !(w->flags & WF_MAPPED)) {
        X_LOCK();
        XMapWindow(X.dpy, w->wid);
        w->flags |= WF_MAPPED;
        X_UNLOCK();
    }
    else if (!val && w->flags & WF_MAPPED) {
        X_LOCK();
        XUnmapWindow(X.dpy, w->wid);
        w->flags &= ~WF_MAPPED;
        X_UNLOCK();
    }
}

EXPORT void
lwi_move(lwi_window_t *w, int px, int py)
{
    X_LOCK();

    XMoveWindow(X.dpy, w->wid, px, py);
    w->x = px;
    w->y = py;

    /*
      XMoveWindow may be ignored when the window is not mapped,
      so we need to set the position hints.
    */
    if (!(w->flags & WF_MAPPED)) {
        w->size_hints.flags |= PPosition;
        w->size_hints.x = px;
        w->size_hints.y = py;
        XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);
        w->size_hints.flags &= ~PPosition;
    }

    X_UNLOCK();
}

EXPORT void
lwi_size(lwi_window_t *w, unsigned sx, unsigned sy)
{
    XWindowChanges changes = {.width=sx, .height=sy};

    X_LOCK();

    XConfigureWindow(X.dpy, w->wid, CWWidth | CWHeight, &changes);

    /* just in case */
    w->size_hints.width = sx;
    w->size_hints.height = sy;
    XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);

    X_UNLOCK();
}

EXPORT void
lwi_min_size(lwi_window_t *w, unsigned sx, unsigned sy)
{
    X_LOCK();
    if (sx == 0 && sy == 0) {
        w->size_hints.flags &= ~PMinSize;
    }
    else {
        w->size_hints.flags |= PMinSize;
        w->size_hints.min_width = sx;
        w->size_hints.min_height = sy;
    }
    XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);
    X_UNLOCK();
}

EXPORT void
lwi_max_size(lwi_window_t *w, unsigned sx, unsigned sy)
{
    X_LOCK();
    if (sx == 0 && sy == 0) {
        w->size_hints.flags &= ~PMaxSize;
    }
    else {
        w->size_hints.flags |= PMaxSize;
        w->size_hints.max_width = sx;
        w->size_hints.max_height = sy;
    }
    XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);
    X_UNLOCK();
}

EXPORT void
lwi_fix_size(lwi_window_t *w, unsigned sx, unsigned sy)
{
    X_LOCK();
    if (sx == 0 && sy == 0) {
        w->size_hints.flags &= ~(PMinSize | PMaxSize);
    }
    else {
        w->size_hints.flags |= PMinSize | PMaxSize;
        w->size_hints.min_width = sx;
        w->size_hints.min_height = sy;
        w->size_hints.max_width = sx;
        w->size_hints.max_height = sy;
    }
    XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);
    X_UNLOCK();
}

EXPORT void
lwi_step_size(lwi_window_t *w, unsigned sx, unsigned sy)
{
    X_LOCK();
    if (sx == 0 && sy == 0) {
        w->size_hints.flags &= ~PResizeInc;
    }
    else {
        w->size_hints.flags |= PResizeInc;
        w->size_hints.width_inc = sx;
        w->size_hints.height_inc = sy;
    }
    XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);
    X_UNLOCK();
}

EXPORT void
lwi_aspect_ratio(lwi_window_t *w, unsigned sx, unsigned sy)
{
    X_LOCK();
    if (sx == 0 && sy == 0) {
        w->size_hints.flags &= ~PAspect;
    }
    else {
        w->size_hints.flags |= PAspect;
        w->size_hints.min_aspect.x = sx;
        w->size_hints.min_aspect.y = sy;
        w->size_hints.max_aspect.x = sx;
        w->size_hints.max_aspect.y = sy;
    }
    XSetWMNormalHints(X.dpy, w->wid, &w->size_hints);
    X_UNLOCK();
}


void
__lwi_enable_frame(lwi_window_t *w)
{
    XDeleteProperty(X.dpy, w->wid, X_ATOM(_MOTIF_WM_HINTS));
}

void
__lwi_disable_frame(lwi_window_t *w)
{

    static const uint32_t mwm_hints[5] = { 2, 0, 0, 0, 0 };
    XChangeProperty(
        X.dpy, w->wid,
        X_ATOM(_MOTIF_WM_HINTS), X_ATOM(_MOTIF_WM_HINTS), 32,
        PropModeReplace, (void *) mwm_hints, NELEMS(mwm_hints));
}

EXPORT void
lwi_framed(lwi_window_t *w, int val)
{
    X_LOCK();
    if (val) {
        __lwi_enable_frame(w);
        w->flags |= WF_FRAMED;
    }
    else {
        __lwi_disable_frame(w);
        w->flags &= ~WF_FRAMED;
    }
    X_UNLOCK();
}

/*
  Manages _NET_WM_STATE property (array of atoms) which requires
  sending a message to the window manager.

  http://standards.freedesktop.org/wm-spec/1.4/ar01s05.html#id2569140
*/
enum {
    _REMOVE = 0,  /* remove/unset property */
    _ADD    = 1,  /* add/set property */
    _TOGGLE = 2,  /* toggle property  */
};
static void
change_net_wm_state(lwi_window_t *w, int op, Atom prop)
{
    const XClientMessageEvent evt = {
        .type = ClientMessage,
        .serial = 0,
        .send_event = True,
        .window = w->wid,
        .message_type = X_ATOM(_NET_WM_STATE),
        .format = 32,
        .data.l = {op, prop, 0, 0 }
    };
    XSendEvent(
        X.dpy, DefaultRootWindow(X.dpy), False,
        SubstructureRedirectMask | SubstructureNotifyMask,
        (void *) &evt);
}

void
__lwi_enable_full_screen(lwi_window_t *w)
{
    change_net_wm_state(w, _ADD, X_ATOM(_NET_WM_STATE_FULLSCREEN));
}

void
__lwi_disable_full_screen(lwi_window_t *w)
{
    change_net_wm_state(w, _REMOVE, X_ATOM(_NET_WM_STATE_FULLSCREEN));
}

EXPORT void
lwi_full_screen(lwi_window_t *w, int val)
{
    X_LOCK();
    if (val && !(w->flags & WF_FULL_SCREEN)) {
        __lwi_enable_full_screen(w);
        w->flags |= WF_FULL_SCREEN;
    }
    else if (!val && w->flags & WF_FULL_SCREEN) {
        __lwi_disable_full_screen(w);
        w->flags &= ~WF_FULL_SCREEN;
    }
    X_UNLOCK();
}

EXPORT void
lwi_title_utf8(lwi_window_t *w, const char *title)
{
    X_LOCK();
    XChangeProperty(
        X.dpy, w->wid, X_ATOM(_NET_WM_NAME), X_ATOM(UTF8_STRING), 8,
        PropModeReplace, (void *) title, strlen(title));
    XChangeProperty(
        X.dpy, w->wid, XA_WM_NAME, XA_STRING, 8,
        PropModeReplace, (void *) title, strlen(title));
    X_UNLOCK();
}

EXPORT void
lwi_clear_colour(lwi_window_t *w, lwi_colour_t colour)
{
    X_LOCK();
    XSetWindowAttributes attrs = {.background_pixel=colour};
    XChangeWindowAttributes(X.dpy, w->wid, CWBackPixel, &attrs);
    w->canvas.clear_colour = colour;
    X_UNLOCK();
}

EXPORT void
lwi_cursor_visible(int val)
{
    if (X_HAS_FIXES) {
        X_LOCK();
        if (val)
            XFixesShowCursor(X.dpy, DefaultRootWindow(X.dpy));
        else
            XFixesHideCursor(X.dpy, DefaultRootWindow(X.dpy));
        X_UNLOCK();
    }
}
