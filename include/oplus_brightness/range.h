#ifndef OPLUS_BRIGHTNESS_RANGE_H
#define OPLUS_BRIGHTNESS_RANGE_H

#include <stdbool.h>

/*
 * Logical (input) brightness range used by "default mode" (OS15+).
 *
 * Seeded from persist.sys.oplus.brightness.range.{min,max} (or the
 * OB_FALLBACK_{MIN,MAX} constants), then optionally "locked" once the
 * ROM's own IR / multi-brightness detection
 * (sys.oplus.multibrightness[.min]) reports a valid range — at which
 * point it's cached back into the persist.* range props and never
 * refreshed again for the lifetime of the process.
 */
typedef struct {
    int min;
    int max;
    bool locked;
} ob_brightness_range_t;

/* Initializes from persisted range props (or fallback constants). */
void ob_range_init(ob_brightness_range_t *range, bool dbg);

/* Re-checks live IR/multibrightness detection and locks the range in
 * if available. No-op once already locked. */
void ob_range_refresh(ob_brightness_range_t *range, bool dbg);

#endif /* OPLUS_BRIGHTNESS_RANGE_H */
