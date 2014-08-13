#include <GL/gl.h>
#include <stdio.h>
#include "lwi_private.h"

EXPORT unsigned
lwi_get_window_count(void)
{
    return LWI.window_count;
}

EXPORT int
lwi_should_exit(void)
{
    return LWI.window_count == 0 || (LWI.flags & LWI_FLAG_EXIT);
}

EXPORT int
lwi_gl_mainloop(lwi_window_t *win, void *context, lwi_render_func_t render_func, lwi_event_func_t event_func)
{
    unsigned fps_count = 0;
    lwi_time_t frame_time = LWI_SECOND / lwi_screen_get_refresh_rate();
    lwi_time_t prev_time, fps_start_time;
    prev_time = fps_start_time = lwi_time();

    lwi_gl_set_context(win);
    int has_vsync = lwi_gl_has_vsync(win);

    if (!has_vsync)
        NOTICE("Warning: No vsync available");

    while (1) {
        render_func(context);
        glFinish();

        fps_count++;
        lwi_time_t fps_current_time = lwi_time();
        if (fps_current_time - fps_start_time > LWI_SECOND * 2) {
            NOTICE("Framerate: %u FPS", fps_count / 2);
            fps_count = 0;
            fps_start_time = fps_current_time;
        }

        do {
            lwi_event_t evt;
            while (lwi_poll(&evt))
                event_func(context, &evt);
            if (has_vsync)
                break;
        } while (lwi_wait_until(frame_time - (lwi_time() - prev_time)));
        prev_time = lwi_time();

        if (lwi_should_exit())
            return LWI.exitcode;

        lwi_gl_swap(win);
    }
}

/* TODO: locking */
void
lwi_exit(int exitcode)
{
    LWI.flags |= LWI_FLAG_EXIT;
    LWI.exitcode = exitcode;
}
