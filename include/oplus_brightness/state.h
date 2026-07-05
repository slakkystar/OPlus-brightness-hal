#ifndef OPLUS_BRIGHTNESS_STATE_H
#define OPLUS_BRIGHTNESS_STATE_H

#include <stdbool.h>

#include "oplus_brightness/range.h"

/*
 * Reads the framework's current logical brightness value from
 * debug.tracing.screen_brightness.
 *
 * is_float:
 *   false -> value is parsed as an int, taking only the leading integer
 *            portion (e.g. "2418.0" -> 2418).
 *   true  -> value is parsed as a float in [0.0, 1.0] and mapped onto
 *            [range->min, range->max].
 *
 * Returns -1 if the value is 0 / unavailable (caller should keep the
 * previous brightness rather than writing 0).
 */
int ob_get_prop_brightness(const ob_brightness_range_t *range, bool is_float);

/*
 * Reads debug.tracing.screen_state:
 *   0 = OFF, 1 = OFF (AOD), 2 = ON, 3 = DOZE (AOD), 4 = DOZE_SUSPEND
 * Defaults to 2 (ON) if unset/unparseable.
 */
int ob_get_screen_state(void);

#endif /* OPLUS_BRIGHTNESS_STATE_H */
