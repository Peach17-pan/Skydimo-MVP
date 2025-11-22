#include "mode_manager.h"

static mode_t cur = MODE_CLOCK;

mode_t mode_get(void) { return cur; }
mode_t mode_next(void)
{
    cur = (cur + 1) % MODE_MAX;
    return cur;
}