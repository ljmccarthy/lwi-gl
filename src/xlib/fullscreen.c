#include <unistd.h>
#include "lwi_private.h"

static XF86VidModeModeInfo *
get_current_mode(void)
{
    int dotclock;
    XF86VidModeModeLine modeline;

    XF86VidModeGetModeLine(X.dpy, X.screen_nr, &dotclock, &modeline);

    for (int i = 0; i < X.num_modes; i++) {
        if (X.modes[i]->hdisplay == modeline.hdisplay
         && X.modes[i]->vdisplay == modeline.vdisplay
         && X.modes[i]->dotclock == (unsigned)dotclock)
            return X.modes[i];
    }
    return NULL;
}

static XF86VidModeModeInfo *
find_video_mode(unsigned width, unsigned height)
{
    for (int i = 0; i < X.num_modes; i++) {
        if (X.modes[i]->hdisplay == width
         && X.modes[i]->vdisplay == height) {
            printf("  vsyncstart=%u  vsyncend=%u  flags=%08x\n",
                X.modes[i]->vsyncstart, X.modes[i]->vsyncend, X.modes[i]->flags);
         }
    }

    for (int i = X.num_modes-1; i >= 0; i--) {
        if (X.modes[i]->hdisplay == width
         && X.modes[i]->vdisplay == height)
            return X.modes[i];
    }
    return NULL;
}

static void
grab_pointer(Window wid)
{
    while (XGrabPointer(
            X.dpy, wid, True,
            ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask,
            GrabModeAsync, GrabModeAsync, wid, None, CurrentTime) != GrabSuccess)
        usleep(100);
}

static void
restore_mode_window(void)
{
    /* release input */
    XUngrabPointer(X.dpy, CurrentTime);
    XUngrabKeyboard(X.dpy, CurrentTime);

    /* restore window configuration */
    lwi_window_t *w = X.mode_window;
    /*
      With ATI Catalyst driver, the screen is corrupted unless
      the window is unmapped and remapped.
      TODO: Test with other drivers.
    */
    XUnmapWindow(X.dpy, w->wid);
    if (w->flags & WF_FRAMED)
        __lwi_enable_frame(w);
    if (w->flags & WF_MAPPED)
        XMapRaised(X.dpy, w->wid);
    if (w->flags & WF_FULL_SCREEN)
        __lwi_enable_full_screen(w);

    X.mode_window = NULL;
}

/*
  FIXME: This is a mess.
  Error conditions might not be handled properly.
  Restoring to regular window mode is broken and window properties are not restored.
*/
EXPORT int
lwi_screen_set_mode(lwi_window_t *w, unsigned width, unsigned height)
{
    if (!X_HAS_VIDMODE)
        return 0;

    X_LOCK();

    XF86VidModeModeInfo *current_mode = X.original_mode;
    if (!current_mode) {
        current_mode = get_current_mode();
        if (!current_mode) {
            X_UNLOCK();
            return 0;
        }
    }

    XF86VidModeModeInfo *mode = find_video_mode(width, height);
    if (!mode) {
        X_UNLOCK();
        return 0;
    }

    int x, y;
    Window child;

    /* configure window */
    __lwi_disable_frame(w);
    __lwi_disable_full_screen(w);  // since screen size may be different to mode size
    XMapRaised(X.dpy, w->wid);
    XTranslateCoordinates(X.dpy, w->wid, DefaultRootWindow(X.dpy), 0, 0, &x, &y, &child);
    XMoveResizeWindow(X.dpy, w->wid, -x, -y, width, height);

    /* grab input */
    grab_pointer(w->wid);
    XWarpPointer(X.dpy, None, DefaultRootWindow(X.dpy), 0, 0, 0, 0, width/2, height/2);
    XSetInputFocus(X.dpy, w->wid, RevertToNone, CurrentTime);
    XGrabKeyboard(X.dpy, w->wid, False, GrabModeAsync, GrabModeAsync, CurrentTime);

    /* switch mode */
    if (!XF86VidModeSwitchToMode(X.dpy, X.screen_nr, mode)) {
        X_UNLOCK();
        return 0;
    }
    XF86VidModeSetViewPort(X.dpy, X.screen_nr, 0, 0);

    X.original_mode = current_mode;
    X.mode_window = w;
    XSync(X.dpy, False);

    X_UNLOCK();
    return 1;
}

EXPORT void
lwi_screen_restore_mode(void)
{
    if (!X_HAS_VIDMODE)
        return;

    X_LOCK();
    if (X.original_mode) {
        NOTICE("Restoring video mode");
        XF86VidModeSwitchToMode(X.dpy, X.screen_nr, X.original_mode);
        restore_mode_window();
        X.original_mode = NULL;
        XSync(X.dpy, False);
    }
    X_UNLOCK();
}

static inline int
refresh_rate(XF86VidModeModeInfo *mode)
{
    int rate = (mode->dotclock * 1000) / (mode->htotal * mode->vtotal);
    /* Low resolutions can produce fast refresh rates,
       but we want something reasonable. */
    while (rate > 100)
        rate /= 2;
    /* Round to nearest multiple of 5 */
    return (rate + 3) / 5 * 5;
}

#define DEFAULT_REFRESH_RATE 60

EXPORT unsigned
lwi_screen_get_refresh_rate(void)
{
    if (!X_HAS_VIDMODE)
        return DEFAULT_REFRESH_RATE;
    X_LOCK();
    XF86VidModeModeInfo *mode = get_current_mode();
    X_UNLOCK();
    if (!mode)
        return DEFAULT_REFRESH_RATE;
    return mode ? refresh_rate(mode) : DEFAULT_REFRESH_RATE;
}

EXPORT void
lwi_screen_get_size(unsigned *width_out, unsigned *height_out)
{
    int width, height;
    XF86VidModeModeInfo *mode = 0;

    if (X_HAS_VIDMODE) {
        X_LOCK();
        mode = get_current_mode();
        X_UNLOCK();
    }

    if (mode) {
        width = mode->hdisplay;
        height = mode->vdisplay;
    }
    else {
        width = DisplayWidth(X.dpy, DefaultScreen(X.dpy));
        height = DisplayHeight(X.dpy, DefaultScreen(X.dpy));
    }

    if (width_out)
        *width_out = width;
    if (height)
        *height_out = height;
}
