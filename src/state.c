#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "oplus_brightness/constants.h"
#include "oplus_brightness/paths.h"
#include "oplus_brightness/properties.h"
#include "oplus_brightness/state.h"

int ob_get_prop_brightness(const ob_brightness_range_t *range, bool is_float) {
    char buf[64];
    if (!ob_get_prop(OB_PROP_SCREEN_BRIGHTNESS, buf, sizeof(buf))) {
        return OB_FALLBACK_MIN;
    }

    if (is_float) {
        char *end = NULL;
        float f = strtof(buf, &end);
        if (end == buf) {
            return OB_FALLBACK_MIN;
        }
        if (f == 0.0f) {
            return -1; /* skip write, keep previous */
        }
        if (f < 0.0f) f = 0.0f;
        if (f > 1.0f) f = 1.0f;
        return (int)lroundf((float)range->min + f * (float)(range->max - range->min));
    }

    /* Take only the leading integer portion, e.g. "2418.0" -> 2418. */
    char intbuf[32];
    size_t n = 0;
    for (size_t i = 0; buf[i] != '\0' && buf[i] != '.' && n < sizeof(intbuf) - 1; i++) {
        intbuf[n++] = buf[i];
    }
    intbuf[n] = '\0';
    if (n == 0) {
        return OB_FALLBACK_MIN;
    }

    char *end = NULL;
    long v = strtol(intbuf, &end, 10);
    if (end == intbuf) {
        return OB_FALLBACK_MIN;
    }
    if (v == 0) {
        return -1; /* skip write, keep previous */
    }
    return (int)v;
}

int ob_get_screen_state(void) {
    int v;
    if (ob_get_prop_int(OB_PROP_SCREEN_STATE, &v)) {
        return v;
    }
    return 2; /* default to ON */
}
