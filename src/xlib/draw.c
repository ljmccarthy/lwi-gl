#include <stdlib.h>
#include "lwi_private.h"

static void
prepare_canvas(lwi_canvas_t *d, unsigned width, unsigned height)
{
    if (d->width != width || d->height != height) {
#ifdef LWI_BUILD_CAIRO
        if (d->cairo) {
            cairo_surface_destroy(d->cairo);
            d->cairo = NULL;
        }
#endif
        d->width = width;
        d->height = height;
    }
}

EXPORT lwi_canvas_t *
lwi_draw_on_window(lwi_window_t *w)
{
    lwi_canvas_t *d = &w->canvas;
    X_LOCK();
    if (pthread_mutex_trylock(&d->lock) != 0) {
        X_UNLOCK();
        return NULL;
    }
    prepare_canvas(d, w->width, w->height);
    X_UNLOCK();
    return d;
}

EXPORT void
lwi_draw_end(lwi_canvas_t *d)
{
    X_LOCK();

#if 0
    if (d->pid) {
        XRenderFreePicture(X.dpy, d->pid);
        d->pid = 0;
    }
#endif

    if (d->did != d->did_front) {
//        XRectangle clip[1] = {{0, 0, d->width, d->height}};
//        XSetClipRectangles(X.dpy, d->cid, 0, 0, clip, 1, Unsorted);

        XdbeSwapInfo swap[1] = {{d->did_front, XdbeBackground}};
        XdbeSwapBuffers(X.dpy, swap, 1);
    }

    pthread_mutex_unlock(&d->lock);
    X_UNLOCK();
}

EXPORT void
lwi_draw_swap_rect(lwi_canvas_t *d, int px, int py, unsigned sx, unsigned sy)
{
//    printf("swap rect: %d,%d %u,%u\n", px, py, sx, sy);
    if (d->did != d->did_front) {
        XCopyArea(
            X.dpy, d->did, d->did_front, d->cid,
            px, py, px, py, sx, sy);
    }
}

EXPORT void
lwi_draw_swap(lwi_canvas_t *d)
{
    lwi_draw_swap_rect(d, 0, 0, d->width, d->height);
}

EXPORT void
lwi_draw_clear_rect(lwi_canvas_t *d, int px, int py, unsigned sx, unsigned sy)
{
    XSetBackground(X.dpy, d->cid, d->clear_colour);
    XFillRectangle(X.dpy, d->did, d->cid, px, py, sx, sy);
#ifdef LWI_BUILD_CAIRO
    if (d->cairo)
        cairo_surface_mark_dirty_rectangle(d->cairo, px, py, sx, sy);
#endif
}

EXPORT void
lwi_draw_clear(lwi_canvas_t *d)
{
    lwi_draw_clear_rect(d, 0, 0, d->width, d->height);
}

#ifdef LWI_BUILD_CAIRO
EXPORT cairo_surface_t *
lwi_draw_cairo_surface(lwi_canvas_t *d)
{
    if (!d->cairo)
        d->cairo = cairo_xlib_surface_create(
            X.dpy, d->did, X.visual, d->width, d->height);
    return d->cairo;
}
#endif

#if 0
EXPORT Picture
lwi_draw_render_picture(lwi_canvas_t *d)
{
    if (X_HAS_RENDER && !d->pid) {
        d->pid = XRenderCreatePicture(
            X.dpy, d->did, X.pictformat, 0, NULL);
    }
    return d->pid;
}
#endif

EXPORT int
lwi_gl_set_context(lwi_window_t *w)
{
    if (!w) {
        X_LOCK();
        glXMakeCurrent(X.dpy, None, NULL);
        X_UNLOCK();
        return 0;
    }

    if (!w->glx_vinfo)
        return 0;

    int ret = 0;
    X_LOCK();

    if (!w->glx_cid)
        w->glx_cid = glXCreateContext(X.dpy, w->glx_vinfo, NULL, True);
    if (w->glx_cid)
        ret = glXMakeCurrent(X.dpy, w->wid, w->glx_cid);

    /* enable vsync if available */
    if ((X.glXSwapIntervalSGI && X.glXSwapIntervalSGI(1) == 0)
        || (X.glXSwapIntervalMESA && X.glXSwapIntervalMESA(1) == 0))
        w->flags |= WF_VSYNC_SET;

    X_UNLOCK();
    return ret;
}

EXPORT void
lwi_gl_swap(lwi_window_t *w)
{
    if (w->glx_cid) {
        X_LOCK();
        glFinish();

        if (w->flags & WF_VSYNC_SET) {
            glXSwapBuffers(X.dpy, w->wid);
        }
        else if (X.glXGetVideoSyncSGI && X.glXWaitVideoSyncSGI) {
            unsigned int before = 0, after = 0;
            if (X.glXGetVideoSyncSGI(&before) == 0)
                X.glXWaitVideoSyncSGI(2, (before + 1) % 2, &after);
            glXSwapBuffers(X.dpy, w->wid);
        }
        else if (X.glXGetSyncValuesOML && X.glXSwapBuffersMscOML) {
            int64_t ust, msc, sbc;
            if (X.glXGetSyncValuesOML(X.dpy, w->wid, &ust, &msc, &sbc))
                X.glXSwapBuffersMscOML(X.dpy, w->wid, msc, 0, 0);
        }
        else {
            /* no vsync available */
            glXSwapBuffers(X.dpy, w->wid);
        }
        X_UNLOCK();
    }
}

EXPORT int
lwi_gl_has_vsync(lwi_window_t *w)
{
    return w->glx_vinfo && (
        (w->flags & WF_VSYNC_SET)
        || (X.glXGetVideoSyncSGI && X.glXWaitVideoSyncSGI)
        || ((X.glXGetSyncValuesOML && X.glXSwapBuffersMscOML)));
}

/*

lwi_canvas_t *
lwi_get_canvas(lwi_window_t *w);

void
lwi_invalidate(lwi_window_t *w, int x, int y, unsigned width, unsigned height);

void
lwi_validate(lwi_window_t *w, int x, int y, unsigned width, unsigned height);

lwi_canvas_t *
lwi_server_image_get_canvas(lwi_server_image_t *si);

*/
