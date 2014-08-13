#include <GL/gl.h>
#include <GL/glu.h>
#include <lwi.h>
#include <math.h>
#include <stdio.h>

#define WIDTH 640
#define HEIGHT 480

static void
handle_render(void *ctx)
{
    static double red = 1.0;

    glViewport(0, 0, WIDTH, HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, HEIGHT, 0);

    glBegin(GL_QUADS);
    glColor3f(fabs(1.0 - red), 0.0, 0.0);
    glVertex2i(10, 10);
    glColor3f(0.0, 1.0, 0.0);
    glVertex2i(WIDTH - 10, 10);
    glColor3f(0.0, 0.0, 1.0);
    glVertex2i(WIDTH - 10, HEIGHT - 10);
    glColor3f(1.0, 1.0, 0.0);
    glVertex2i(10, HEIGHT - 10);

    glColor3f(1.0, 1.0, 0.0);
    glVertex2i(20, 20);
    glColor3f(0.0, 0.0, 1.0);
    glVertex2i(WIDTH - 20, 20);
    glColor3f(0.0, 1.0, 0.0);
    glVertex2i(WIDTH - 20, HEIGHT - 20);
    glColor3f(fabs(1.0 - red), 0.0, 0.0);
    glVertex2i(20, HEIGHT - 20);

    glColor3f(0.0, 0.0, 1.0);
    glVertex2i(30, 30);
    glColor3f(1.0, 1.0, 0.0);
    glVertex2i(WIDTH - 30, 30);
    glColor3f(fabs(1.0 - red), 0.0, 0.0);
    glVertex2i(WIDTH - 30, HEIGHT - 30);
    glColor3f(0.0, 1.0, 0.0);
    glVertex2i(30, HEIGHT - 30);
    glEnd();

    red = fmod(red + 0.005, 2.0);
}

static void
handle_event(void *ctx, lwi_event_t *evt)
{
    lwi_window_t *w = evt->window;

    switch (evt->type) {
    case LWI_WINDOW_CLOSE:
        lwi_close(w);
        break;
    default:
        break;
    }
}

static void
init_gl(void)
{
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_TEXTURE_3D);

    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int
main()
{
    if (!lwi_init())
        return 1;

    lwi_window_t *win = lwi_gl_open(WIDTH, HEIGHT, 0);
    if (!win)
        return 1;

    lwi_gl_set_context(win);
    init_gl();

    lwi_title(win, "OpenGL 2D");
    lwi_fix_size(win, WIDTH, HEIGHT);
    lwi_visible(win, 1);

    return lwi_gl_mainloop(win, NULL, handle_render, handle_event);
}
