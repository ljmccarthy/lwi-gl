#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include "lwi_private.h"

static XImage *
create_xshmimage(unsigned sx, unsigned sy, XShmSegmentInfo *shm)
{
    XImage *img = XShmCreateImage(
        X.dpy, X.visual, DefaultDepth(X.dpy, X.screen_nr),
        ZPixmap, 0, shm, sx, sy);
    if (!img)
        goto err0;

    shm->shmid = shmget(IPC_PRIVATE,
        img->bytes_per_line * img->height, IPC_CREAT | 0777);
    if (shm->shmid < 0)
        goto err1;

    shm->shmaddr = shmat(shm->shmid, NULL, 0);
    if (shm->shmaddr == (void *) -1)
        goto err2;

    img->data = shm->shmaddr;
    shm->readOnly = False;
    if (!XShmAttach(X.dpy, shm))
        goto err3;

    return img;
err3:
    shmdt(shm->shmaddr);
err2:
    shmctl(shm->shmid, IPC_RMID, NULL);
err1:
    XDestroyImage(img);
err0:
    return NULL;
}

static XImage *
create_ximage(unsigned sx, unsigned sy)
{
    XImage *img = XCreateImage(
        X.dpy, X.visual, DefaultDepth(X.dpy, X.screen_nr),
        ZPixmap, 0, 0, sx, sy, BitmapPad(X.dpy), 0);
    if (!img)
        goto err0;

    img->data = malloc(img->bytes_per_line * img->height);
    if (!img->data)
        goto err1;

    return img;
err1:
    XDestroyImage(img);
err0:
    return NULL;
}

EXPORT lwi_client_image_t *
lwi_client_image_create(unsigned sx, unsigned sy)
{
    lwi_client_image_t *ci = malloc(sizeof(lwi_client_image_t));
    if (!ci)
        return NULL;

    memset(ci, 0, sizeof(lwi_client_image_t));

    X_LOCK();
    if (X_HAS_SHM)
        ci->ximg = create_xshmimage(sx, sy, &ci->shm);
    else
        ci->ximg = create_ximage(sx, sy);
    X_UNLOCK();

    if (!ci->ximg) {
        free(ci);
        return NULL;
    }
    return ci;
}

EXPORT void
lwi_client_image_destroy(lwi_client_image_t *ci)
{
    X_LOCK();
    if (ci->shm.shmaddr) {
        XShmDetach(X.dpy, &ci->shm);
        XDestroyImage(ci->ximg);
        shmdt(ci->shm.shmaddr);
        shmctl(ci->shm.shmid, IPC_RMID, NULL);
    }
    else {
        free(ci->ximg->data);
        ci->ximg->data = NULL;
        XDestroyImage(ci->ximg);
    }
    X_UNLOCK();

    memset(ci, 0, sizeof(lwi_client_image_t));
    free(ci);
}

EXPORT void
lwi_client_image_blit(
    lwi_canvas_t *d, int x, int y,
    lwi_client_image_t *ci, int ix, int iy, unsigned width, unsigned height)
{
    X_LOCK();
    if (ci->shm.shmaddr) {
        XShmPutImage(
            X.dpy, d->did, d->cid, ci->ximg, ix, iy,
            x, y, width, height, False);
        XSync(X.dpy, False);
    }
    else {
        XPutImage(
            X.dpy, d->did, d->cid, ci->ximg, ix, iy,
            x, y, width, height);
    }
    X_UNLOCK();
}

EXPORT void
lwi_client_image_put(lwi_canvas_t *d, int x, int y, lwi_client_image_t *ci)
{
    lwi_client_image_blit(d, x, y, ci, 0, 0, ci->ximg->width, ci->ximg->height);
}

EXPORT void *
lwi_client_image_data(lwi_client_image_t *ci)
{
    return ci->ximg->data;
}
