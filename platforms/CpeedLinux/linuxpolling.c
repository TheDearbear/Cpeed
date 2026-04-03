#include <stdbool.h>

#include "linuxpolling.h"

#define SLEEP_TIME_NSEC 50000

gint poll_fd_size;
GArray* g_poll_fds;

uint32_t poll_glib_events(void* context) {
    bool* is_running = (bool*)context;

    while (*is_running) {
        struct timespec ts = {
            .tv_sec = 0,
            .tv_nsec = SLEEP_TIME_NSEC
        };

        nanosleep(&ts, 0);

        GMainContext* ctx = g_main_context_default();

        if (!g_main_context_acquire(ctx)) {
            continue;
        }

        gint fds;
        gint timeout;
        gint max_priority;
        if (g_main_context_prepare(ctx, &max_priority)) {
            g_main_context_dispatch(ctx);
        }

        GPollFD* arr = &g_array_index(g_poll_fds, GPollFD, 0);
        guint size = g_poll_fds->len;

        fds = g_main_context_query(ctx, max_priority, &timeout, arr, size);

        if (size < fds) {

            g_poll_fds = g_array_set_size(g_poll_fds, fds);
            arr = &g_array_index(g_poll_fds, GPollFD, 0);
            size = g_poll_fds->len;

            fds = g_main_context_query(ctx, max_priority, &timeout, arr, size);
        }

        if (g_main_context_check(ctx, max_priority, arr, size)) {
            g_main_context_dispatch(ctx);
        }

        g_main_context_release(ctx);
    }

    return 0;
}
