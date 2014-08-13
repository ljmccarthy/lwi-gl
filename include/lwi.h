#ifndef LWI_H
#define LWI_H

#ifdef LWI_WIN32
    #ifdef WIN32_LEAN_AND_MEAN
        #include <windows.h>
    #else
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #undef WIN32_LEAN_AND_MEAN
    #endif
#endif

#ifdef LWI_X11
    #include <X11/Xlib.h>
#endif

#ifdef LWI_CAIRO
    #include <cairo/cairo.h>
#endif

#ifdef __cplusplus
namespace lwi_capi {
extern "C" {
#endif

typedef struct lwi_window_t lwi_window_t;
typedef struct lwi_canvas_t lwi_canvas_t;
typedef struct lwi_client_image_t lwi_client_image_t;
typedef struct lwi_server_image_t lwi_server_image_t;

typedef unsigned int lwi_colour_t;
typedef unsigned int lwi_key_t;
typedef long long lwi_time_t;

typedef enum lwi_event_type_t {
    LWI_KEY_DOWN         = 1,
    LWI_KEY_UP           = 2,
    LWI_KEY_REPEAT       = 3,
    LWI_BUTTON_DOWN      = 4,
    LWI_BUTTON_UP        = 5,
    LWI_MOUSE_MOVE       = 6,
    LWI_MOUSE_ENTER      = 7,
    LWI_MOUSE_LEAVE      = 8,
    LWI_WINDOW_EXPOSE    = 9,
    LWI_WINDOW_HIDE      = 10,
    LWI_WINDOW_SHOW      = 11,
    LWI_WINDOW_MOVE      = 12,
    LWI_WINDOW_RESIZE    = 13,
    LWI_WINDOW_FOCUS_IN  = 14,
    LWI_WINDOW_FOCUS_OUT = 15,
    LWI_WINDOW_CLOSE     = 16,
    LWI_SHUTDOWN         = 17,
} lwi_event_type_t;

typedef struct lwi_event_t {
    lwi_event_type_t type;
    lwi_window_t *window;
    unsigned int time;
    lwi_key_t key;
    short x, y;
    unsigned short width, height;
} lwi_event_t;

typedef enum lwi_image_format_t {
    LWI_IMAGE_FORMAT_NONE,
    LWI_IMAGE_FORMAT_GRAY,
    LWI_IMAGE_FORMAT_GRAY_ALPHA,
    LWI_IMAGE_FORMAT_RGB,
    LWI_IMAGE_FORMAT_RGBA,
} lwi_image_format_t;

typedef struct lwi_image_t {
    unsigned char *pixel_data;
    lwi_image_format_t format;
    unsigned width;
    unsigned height;
    unsigned stride;
} lwi_image_t;

typedef void (*lwi_render_func_t)(void *ctx);
typedef void (*lwi_event_func_t)(void *ctx, lwi_event_t *evt);

int
lwi_init(void);

void
lwi_shutdown(void);

/* query information */

unsigned
lwi_screen_get_colour_depth(void);

unsigned
lwi_screen_get_refresh_rate(void);

void
lwi_screen_get_size(unsigned *width, unsigned *height);

unsigned
lwi_get_window_count(void);

lwi_time_t
lwi_time(void);

/* window and events */

lwi_window_t *
lwi_open(unsigned sx, unsigned sy);

lwi_window_t *
lwi_gl_open(unsigned sx, unsigned sy, unsigned flags);

void
lwi_close(lwi_window_t *w);

int
lwi_poll(lwi_event_t *evt);

int
lwi_wait(void);

int
lwi_wait_until(lwi_time_t timeout);

void
lwi_exit(int exitcode);

int
lwi_should_exit(void);

/* window configuration */

void
lwi_set_user_data(lwi_window_t *w, void *p);

void *
lwi_get_user_data(lwi_window_t *w);

void
lwi_visible(lwi_window_t *w, int val);

void
lwi_move(lwi_window_t *w, int px, int py);

void
lwi_size(lwi_window_t *w, unsigned sx, unsigned sy);

void
lwi_min_size(lwi_window_t *w, unsigned sx, unsigned sy);

void
lwi_max_size(lwi_window_t *w, unsigned sx, unsigned sy);

void
lwi_fix_size(lwi_window_t *w, unsigned sx, unsigned sy);

void
lwi_step_size(lwi_window_t *w, unsigned sx, unsigned sy);

void
lwi_aspect_ratio(lwi_window_t *w, unsigned sx, unsigned sy);

void
lwi_framed(lwi_window_t *w, int val);

void
lwi_full_screen(lwi_window_t *w, int val);

void
lwi_title_utf8(lwi_window_t *w, const char *title);

void
lwi_title_utf16(lwi_window_t *w, const short *title);

#ifdef LWI_UTF16
    #define lwi_title(_w, _title) lwi_title_utf16(_w, _title)
#else
    #define lwi_title(_w, _title) lwi_title_utf8(_w, _title)
#endif

void
lwi_clear_colour(lwi_window_t *w, lwi_colour_t colour);

void
lwi_cursor_visible(int val);

/* input */

unsigned int
lwi_key_to_unicode(unsigned int key);

/* drawing */

lwi_canvas_t *
lwi_draw_on_window(lwi_window_t *w);

void
lwi_draw_end(lwi_canvas_t *d);

void
lwi_draw_swap(lwi_canvas_t *d);

void
lwi_draw_swap_rect(lwi_canvas_t *d, int px, int py, unsigned sx, unsigned sy);

void
lwi_draw_clear(lwi_canvas_t *d);

void
lwi_draw_clear_rect(lwi_canvas_t *d, int px, int py, unsigned sx, unsigned sy);

/* image loading */

int
lwi_image_read_png(lwi_image_t *img, const char *filename);

/* server-side images */

lwi_server_image_t *
lwi_server_image_create(unsigned width, unsigned height);

lwi_server_image_t *
lwi_server_image_from_client_image(lwi_client_image_t *ci);

void
lwi_server_image_destroy(lwi_server_image_t *si);

void
lwi_server_image_blit(
    lwi_canvas_t *d, int x, int y,
    lwi_server_image_t *si, int ix, int iy, unsigned width, unsigned height);

void
lwi_server_image_put(lwi_canvas_t *d, int x, int y, lwi_server_image_t *si);

/* client-side images */

lwi_client_image_t *
lwi_client_image_create(unsigned sx, unsigned sy);

lwi_client_image_t *
lwi_client_image_from_server_image(lwi_server_image_t *si);

void
lwi_client_image_destroy(lwi_client_image_t *ci);

void
lwi_client_image_blit(
    lwi_canvas_t *d, int x, int y,
    lwi_client_image_t *ci, int ix, int iy, unsigned width, unsigned height);

void
lwi_client_image_put(lwi_canvas_t *d, int x, int y, lwi_client_image_t *ci);

void *
lwi_client_image_data(lwi_client_image_t *ci);

/* video mode */

int
lwi_screen_set_mode(lwi_window_t *w, unsigned width, unsigned height);

void
lwi_screen_restore_mode(void);

#ifdef LWI_CAIRO
cairo_surface_t *
lwi_draw_cairo_surface(lwi_canvas_t *d);
#endif

/* GL */

int
lwi_gl_set_context(lwi_window_t *w);

void
lwi_gl_swap(lwi_window_t *w);

unsigned
lwi_gl_texture_from_image(lwi_image_t *img);

unsigned
lwi_gl_texture_read_png(const char *filename);

int
lwi_gl_has_vsync(lwi_window_t *w);

/* X11 */

#ifdef LWI_X11
Display *
lwi_x11_get_display(void);

int
lwi_x11_get_display_filedes(void);

Window
lwi_x11_get_xid(lwi_window_t *win);
#endif /* LWI_X11 */

#ifdef __cplusplus
} /* extern "C" */
} /* namespace lwi_capi */
#endif

#define LWI_RGB(r,g,b) (((r) & 0xFF) << 16 | (((g) & 0xFF) << 8) | ((b) & 0xFF))
#define LWI_RED(colour) (((colour) >> 16) & 0xFF)
#define LWI_GREEN(colour) (((colour) >> 8) & 0xFF)
#define LWI_BLUE(colour) ((colour) & 0xFF)

#define LWI_KEY(code, mod) ((lwi_key_t)(code) << 16 | (mod))
#define LWI_KEY_CODE(key) ((lwi_key_t)(key) >> 16)
#define LWI_KEY_FLAGS(key) ((lwi_key_t)(key) & 0xFFFF)
#define LWI_KEY_SHIFT (1 << 0)
#define LWI_KEY_CAPS  (1 << 1)
#define LWI_KEY_CTRL  (1 << 2)

#define LWI_SECOND       1000000000LL
#define LWI_MILLI_SECOND 1000000LL
#define LWI_MICRO_SECOND 1000LL
#define LWI_NANO_SECOND  1LL

#define LWI_GL_DEPTH   (1 << 0)
#define LWI_GL_STENCIL (1 << 1)

enum {
    LWI_DND_STRING,
    LWI_DND_URI,
};

#endif /* LWI_H */
