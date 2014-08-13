#include <stdio.h>
#include <lwi.h>
#include <cairo.h>
#include <math.h>

#ifndef LWI_CAIRO
#error
#endif

static void
render(cairo_t *cr)
{
cairo_pattern_t *pat;

pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0);
cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
cairo_rectangle (cr, 0, 0, 256, 256);
cairo_set_source (cr, pat);
cairo_fill (cr);
cairo_pattern_destroy (pat);

pat = cairo_pattern_create_radial (115.2, 102.4, 25.6,
                                   102.4,  102.4, 128.0);
cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
cairo_set_source (cr, pat);
cairo_arc (cr, 128.0, 128.0, 76.8, 0, 2 * M_PI);
cairo_fill (cr);
cairo_pattern_destroy (pat);


cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.3);
cairo_set_line_width(cr, 40.96);
cairo_move_to(cr, 76.8, 84.48);
cairo_rel_line_to(cr, 51.2, -51.2);
cairo_rel_line_to(cr, 51.2, 51.2);
cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
cairo_stroke(cr);

cairo_set_source_rgba(cr, 0.0, 0.5, 0.5, 0.3);
cairo_set_line_width(cr, 40.96);
cairo_move_to(cr, 76.8, 161.28);
cairo_rel_line_to(cr, 51.2, -51.2);
cairo_rel_line_to(cr, 51.2, 51.2);
cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
cairo_stroke(cr);

cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.3);
cairo_set_line_width(cr, 40.96);
cairo_move_to(cr, 76.8, 84.48);
cairo_rel_line_to(cr, 51.2, -51.2);
cairo_rel_line_to(cr, 51.2, 51.2);
cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
cairo_stroke(cr);

cairo_set_source_rgba(cr, 0.5, 0.5, 1.0, 0.3);
cairo_set_line_width(cr, 40.96);
cairo_move_to(cr, 76.8, 238.08);
cairo_rel_line_to(cr, 51.2, -51.2);
cairo_rel_line_to(cr, 51.2, 51.2);
cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
cairo_stroke(cr);
}

#define FPS (LWI_SECOND/60)

int
main()
{
	lwi_init();

	lwi_window_t *w = lwi_open(640, 480);
	if (!w) {
		fprintf(stderr, "can't open window!\n");
		return 1;
	}

	lwi_title_utf8(w, "Cairo Demo");
	lwi_clear_colour(w, LWI_RGB(0, 0, 0xFF));
	lwi_drop_target(w, 1);
	lwi_visible(w, 1);

	lwi_time_t t = lwi_time();
	unsigned count = 0;
	int exposed = 1;

	while (lwi_get_window_count()) {
		if (exposed) {
			exposed = 0;
//			printf("render %d\n", count++);
			lwi_canvas_t *d = lwi_draw_on_window(w);
			cairo_surface_t *cs = lwi_draw_cairo_surface(d);
			cairo_t *cr = cairo_create(cs);
			render(cr);
			cairo_destroy(cr);
			lwi_draw_end(d);
		}
		else
			lwi_wait();
		do {
			lwi_event_t evt;
			while (lwi_poll(&evt)) {
				switch (evt.type) {
				case LWI_WINDOW_EXPOSE:
					exposed = 1;
					break;
				case LWI_WINDOW_CLOSE:
					lwi_close(evt.window);
					return 0;
				default:
					break;
				}
			}
		} while (lwi_wait_until(FPS - (lwi_time() - t)));
		t = lwi_time();
	}
}
