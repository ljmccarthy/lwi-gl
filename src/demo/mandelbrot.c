#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <lwi.h>

#define ZOOM 2
#define SHIFT 30
#define ACCURACY 150

static lwi_client_image_t *img;
static double zoom, xpos, ypos;
static int needs_redraw = 1;
static uint32_t palette[ACCURACY+1];

#define W 640
#define H 480

enum {
	MOUSE_LEFT        = 1,
	MOUSE_MIDDLE      = 2,
	MOUSE_RIGHT       = 3,
	MOUSE_SCROLL_UP   = 4,
	MOUSE_SCROLL_DOWN = 5,
	KEY_ESCAPE        = 9,
	KEY_SPACE         = 65,
	KEY_UP            = 98,
	KEY_LEFT          = 100,
	KEY_RIGHT         = 102,
	KEY_DOWN          = 104,
};

static void
mandelbrot(void)
{
	uint32_t *data = lwi_client_image_data(img);
	#define PIXEL(x,y) data[(x) + (y) * W]

	for (unsigned a = 0; a < W; a++) {
		for (unsigned b = 0; b < H; b++) {
			const double a0 = (a - xpos) / (W * zoom);
			const double b0 = (b - ypos) / (H * zoom);
			double x = 0, y = 0;
			double x2 = x*x, y2 = y*y;
			int i;
			for (i = 0; x2 + y2 < 4 && i < ACCURACY; i++) {
				y = 2 * x * y + b0;
				x = x2 - y2 + a0;
				x2 = x*x;
				y2 = y*y;
			}
			PIXEL(a, b) = palette[i];
		}
	}

	#undef PIXEL
}

static void
randomise_palette(void)
{
	for (size_t i = 0; i < ACCURACY; i++)
		palette[i] = rand();
	palette[ACCURACY] = 0;
}

static void
dispatch(lwi_event_t *evt)
{
	lwi_window_t *w = evt->window;

	switch (evt->type) {
	case LWI_WINDOW_CLOSE:
		lwi_close(w);
		break;
	case LWI_BUTTON_DOWN:
		switch (LWI_KEY_CODE(evt->key)) {
		case MOUSE_LEFT:
			zoom *= ZOOM;
			xpos = (xpos - (evt->x - W/(2*ZOOM))) * ZOOM;
			ypos = (ypos - (evt->y - H/(2*ZOOM))) * ZOOM;
			needs_redraw = 1;
			break;
		case MOUSE_RIGHT:
			zoom /= ZOOM;
			xpos = (xpos + W/2) / ZOOM;
			ypos = (ypos + H/2) / ZOOM;
			needs_redraw = 1;
			break;
		}
		break;
	case LWI_KEY_DOWN:
		switch (LWI_KEY_CODE(evt->key)) {
		case KEY_LEFT:
			xpos += SHIFT;
			needs_redraw = 1;
			break;
		case KEY_RIGHT:
			xpos -= SHIFT;
			needs_redraw = 1;
			break;
		case KEY_UP:
			ypos += SHIFT;
			needs_redraw = 1;
			break;
		case KEY_DOWN:
			ypos -= SHIFT;
			needs_redraw = 1;
			break;
		case KEY_SPACE:
			randomise_palette();
			needs_redraw = 1;
			break;
		case KEY_ESCAPE:
			lwi_close(w);
			break;
		}
		break;
	case LWI_WINDOW_EXPOSE:
		needs_redraw = 1;
		break;
	default:
		break;
	}
}

static void
redraw(lwi_window_t *w)
{
	if (!needs_redraw)
		return;
	
	lwi_canvas_t *d = lwi_draw_on_window(w);
	if (!d)
		return;

	mandelbrot();
	lwi_client_image_blit(d, 0, 0, img, 0, 0, W, H);
//	lwi_draw_flip(d);
	lwi_draw_end(d);

	needs_redraw = 0;
}

int
main()
{
	srand(time(NULL));

	if (!lwi_init())
		return 1;

	img = lwi_client_image_create(W, H);
	if (!img)
		return 1;

	lwi_window_t *win = lwi_open(W, H);
	if (!win)
		return 1;

	zoom = 0.25;
	xpos = W/2;
	ypos = H/2;
	randomise_palette();

	lwi_fix_size(win, W, H);
	lwi_title(win, "Mandelbrot");
	lwi_visible(win, 1);

	while (lwi_get_window_count()) {
		redraw(win);
		lwi_wait();

		lwi_event_t evt;
		while (lwi_poll(&evt))
			dispatch(&evt);
	}
	lwi_client_image_destroy(img);

	return 0;
}
