#include <stdlib.h>
#include <string.h>
#include "lwi_private.h"

EXPORT lwi_server_image_t *
lwi_server_image_create(unsigned width, unsigned height)
{
    lwi_server_image_t *si = malloc(sizeof(lwi_server_image_t));
    if (!si)
        return NULL;

    X_LOCK();
    si->pid = XCreatePixmap(
        X.dpy, DefaultRootWindow(X.dpy), width, height,
        DefaultDepth(X.dpy, X.screen_nr));
    X_UNLOCK();

    si->width = width;
    si->height = height;
    si->depth = DefaultDepth(X.dpy, X.screen_nr);
    return si;
}

EXPORT void
lwi_server_image_destroy(lwi_server_image_t *si)
{
    X_LOCK();
    XFreePixmap(X.dpy, si->pid);
    X_UNLOCK();

    memset(si, 0, sizeof(lwi_server_image_t));
    free(si);
}

EXPORT void
lwi_server_image_blit(
    lwi_canvas_t *d, int x, int y,
    lwi_server_image_t *si, int ix, int iy, unsigned width, unsigned height)
{
    X_LOCK();
    XCopyArea(X.dpy, si->pid, d->did, d->cid, ix, iy, width, height, x, y);
    X_UNLOCK();
}

EXPORT void
lwi_server_image_put(lwi_canvas_t *d, int x, int y, lwi_server_image_t *si)
{
    lwi_server_image_blit(d, x, y, si, 0, 0, si->width, si->height);
}
