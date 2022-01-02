#ifndef LIBCONF_TIMER_H
#define LIBCONF_TIMER_H

#include <time.h>

#define TIMER_START(name) clock_t name ## _auto_timer = clock()

#define TIMER_END(name) printf("Timer '" #name "' took %f ms\n", (((double) clock() - name ## _auto_timer)/CLOCKS_PER_SEC)*1000.0)


#endif //LIBCONF_TIMER_H
