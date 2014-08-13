#include <stdio.h>
#include <lwi.h>

int
main()
{
	while (1) {
		printf("%.2f\n", lwi_time() / (double)LWI_SECOND);
//		sleep(1);
	}
	return 0;
}
