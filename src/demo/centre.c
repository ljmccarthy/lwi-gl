#include <stdio.h>
#include <lwi.h>

#define W 400
#define H 250

int
main()
{
	lwi_init();

	lwi_window_t *w = lwi_open(W, H);
	if (!w) {
		fprintf(stderr, "can't open window!\n");
		return 1;
	}

	unsigned sw, sh;
	lwi_screen_get_size(&sw, &sh);

	lwi_clear_colour(w, LWI_RGB(0xFF, 0, 0));
//	lwi_framed(w, 0);
	lwi_move(w, (sw - W) / 2, (sh - H) / 2);
	lwi_visible(w, 1);

	while (lwi_get_window_count()) {
		lwi_event_t evt;
		while (lwi_poll(&evt)) {
			switch (evt.type) {
			case LWI_KEY_DOWN:
				if (LWI_KEY_CODE(evt.key) == 9) {
					while (1)
						getchar();
//					while (1)
//						;
					lwi_close(w);
					return 0;
				}
				break;
			case LWI_WINDOW_CLOSE:
				lwi_close(evt.window);
				break;
			default:
				break;
			}
		}
		lwi_wait();
	}
}
