#include "mode_manager.h"

static mode_t current_mode = MODE_CLOCK;

mode_t mode_get(void) 
{ 
    return current_mode; 
}

mode_t mode_next(void)
{
    current_mode = (current_mode + 1) % MODE_MAX;
    return current_mode;
}

void mode_set(mode_t new_mode)
{
    if (new_mode < MODE_MAX) {
        current_mode = new_mode;
    }
}