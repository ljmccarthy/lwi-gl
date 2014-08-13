#include <stdio.h>
#include <lwi.h>

int
main()
{
	lwi_init();

	lwi_window_t *w1 = lwi_open(640, 480);
	if (!w1) {
		fprintf(stderr, "can't open window!\n");
		return 1;
	}

	lwi_window_t *w2 = lwi_open(600, 250);
	if (!w2) {
		fprintf(stderr, "can't open window!\n");
		return 1;
	}

	lwi_title_utf8(w1, "LWI Demo ルク");
	lwi_step_size(w1, 20, 5);

	lwi_title_utf8(w2, "Test Window");
	lwi_aspect_ratio(w2, 4, 3);

	lwi_visible(w1, 1);
	lwi_visible(w2, 1);

	while (lwi_get_window_count()) {
		lwi_event_t evt;
		while (lwi_poll(&evt)) {
			printf("event: %d %p\n", evt.type, evt.window);
			switch (evt.type) {
			case LWI_KEY_DOWN:
				printf("Key: %c\n", lwi_key_to_unicode(evt.key));
				break;
			case LWI_WINDOW_CLOSE:
				lwi_close(evt.window);
				break;
			default:
				break;
			}
		}
		lwi_wait();
		printf("wait\n");
	}
}
