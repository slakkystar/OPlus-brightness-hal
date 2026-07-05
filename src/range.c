#include "oplus_brightness/range.h"
#include "oplus_brightness/constants.h"
#include "oplus_brightness/logging.h"
#include "oplus_brightness/paths.h"
#include "oplus_brightness/properties.h"

void ob_range_init(ob_brightness_range_t *range, bool dbg) {
    int pmin, pmax;
    bool have_min = ob_get_prop_int(OB_PROP_RANGE_MIN, &pmin);
    bool have_max = ob_get_prop_int(OB_PROP_RANGE_MAX, &pmax);

    if (have_min && have_max && pmin < pmax) {
        range->min = pmin;
        range->max = pmax;
    } else {
        range->min = OB_FALLBACK_MIN;
        range->max = OB_FALLBACK_MAX;
    }
    range->locked = false;

    if (dbg) {
        ob_log_d("[BrightnessRange] Initialized with range: min=%d, max=%d",
                 range->min, range->max);
    }
}

void ob_range_refresh(ob_brightness_range_t *range, bool dbg) {
    if (range->locked) {
        return;
    }

    int pmin, pmax, rmin, rmax;
    bool have_pmin = ob_get_prop_int(OB_PROP_RANGE_MIN, &pmin);
    bool have_pmax = ob_get_prop_int(OB_PROP_RANGE_MAX, &pmax);
    bool have_rmin = ob_get_prop_int(OB_PROP_SYS_MULTIBRIGHTNESS_MIN, &rmin);
    bool have_rmax = ob_get_prop_int(OB_PROP_SYS_MULTIBRIGHTNESS_MAX, &rmax);

    if (have_rmin && have_rmax && rmin < rmax) {
        range->min = rmin;
        range->max = rmax;
        if (!(have_pmin && pmin == rmin)) {
            ob_set_prop_int(OB_PROP_RANGE_MIN, rmin);
        }
        if (!(have_pmax && pmax == rmax)) {
            ob_set_prop_int(OB_PROP_RANGE_MAX, rmax);
        }
        range->locked = true;
        if (dbg) {
            ob_log_d("[BrightnessRange] Locked from live IR detection: min=%d, max=%d",
                     range->min, range->max);
        }
    } else if (have_pmin && have_pmax && pmin < pmax) {
        range->min = pmin;
        range->max = pmax;
    } else {
        range->min = OB_FALLBACK_MIN;
        range->max = OB_FALLBACK_MAX;
    }

    if (range->min >= range->max) {
        range->min = OB_FALLBACK_MIN;
        range->max = OB_FALLBACK_MAX;
    }
}
