#include <stdio.h>
#include <string.h>

#define HOST_PREVIEW 1
#include "../target/src/codex_monitor.c"

static int expect_text(const char *actual, const char *expected)
{
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "expected '%s', got '%s'\n", expected, actual);
        return 0;
    }
    return 1;
}

int main(void)
{
    struct quota_state state = { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 };
    char text[24];
    if (monitor_status_for(&state, 100U) != MONITOR_WAIT ||
        !expect_text((format_updated(text, &state, 100U), text), "UPDATED ---")) return 1;

    state.valid = 1;
    state.last_receive_target = 100U;
    if (monitor_status_for(&state, 100U) != MONITOR_LIVE ||
        monitor_status_for(&state, 219U) != MONITOR_LIVE ||
        monitor_status_for(&state, 220U) != MONITOR_LIVE ||
        monitor_status_for(&state, 221U) != MONITOR_STALE) return 1;

    state.last_receive_target = 500U;
    if (monitor_status_for(&state, 500U) != MONITOR_LIVE ||
        !expect_text((format_updated(text, &state, 500U), text), "UPDATED 0S AGO")) return 1;
    if (!expect_text((format_updated(text, &state, 508U), text), "UPDATED 8S AGO")) return 1;
    if (!expect_text((format_updated(text, &state, 559U), text), "UPDATED 59S AGO")) return 1;
    if (!expect_text((format_updated(text, &state, 560U), text), "UPDATED 1M AGO")) return 1;
    if (!expect_text((format_updated(text, &state, 619U), text), "UPDATED 1M AGO")) return 1;
    if (!expect_text((format_updated(text, &state, 620U), text), "UPDATED 2M AGO")) return 1;
    if (!expect_text((format_updated(text, &state, 4099U), text), "UPDATED 59M AGO")) return 1;
    if (!expect_text((format_updated(text, &state, 4100U), text), "UPDATED 1H AGO")) return 1;

    puts("status logic self-test PASS");
    return 0;
}
