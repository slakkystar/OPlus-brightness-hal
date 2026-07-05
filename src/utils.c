#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oplus_brightness/constants.h"
#include "oplus_brightness/logging.h"
#include "oplus_brightness/paths.h"
#include "oplus_brightness/properties.h"
#include "oplus_brightness/utils.h"

bool ob_read_file_int(const char *path, int *out) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }

    char buf[64];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    /* Trim leading whitespace, then take the leading run of digits
     * (mirrors the original's "take_while is_digit" behaviour). */
    char *p = buf;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    char digits[32];
    size_t d = 0;
    while (isdigit((unsigned char)*p) && d < sizeof(digits) - 1) {
        digits[d++] = *p++;
    }
    digits[d] = '\0';

    if (d == 0) {
        return false;
    }
    *out = (int)strtol(digits, NULL, 10);
    return true;
}

bool ob_is_panoramic_aod_enabled(bool dbg) {
    FILE *p = popen("settings get secure panoramic_aod_enable 2>/dev/null", "r");
    if (!p) {
        if (dbg) {
            ob_log_e("[DisplayAdaptor] Failed to execute 'settings' command");
        }
        return false;
    }

    char buf[32] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, p);
    pclose(p);
    buf[n] = '\0';

    /* Trim trailing whitespace/newline. */
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
        buf[--n] = '\0';
    }

    if (dbg) {
        ob_log_d("[DisplayAdaptor] panoramic_aod_enable result: '%s'", buf);
    }
    return strcmp(buf, "1") == 0;
}

int ob_get_max_brightness(bool dbg) {
    int custom_max;
    if (ob_get_prop_int(OB_PROP_DEVMAX, &custom_max) && custom_max > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Using custom devmax brightness: %d", custom_max);
        return custom_max;
    }

    int cached_max;
    if (ob_get_prop_int(OB_PROP_HW_MAX, &cached_max)) {
        if (dbg) ob_log_d("[DisplayAdaptor] Using cached hw_max: %d", cached_max);
        return cached_max;
    }

    int val;
    if (ob_read_file_int(OB_PATH_MAX_BRIGHTNESS, &val)) {
        if (dbg) ob_log_d("[DisplayAdaptor] Detected hw_max: %d. Saving to prop.", val);
        ob_set_prop_int(OB_PROP_HW_MAX, val);
        return val;
    }

    int ro_max;
    if (ob_get_prop_int(OB_PROP_RO_HW_MAX, &ro_max) && ro_max > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_max, using ro prop: %d", ro_max);
        return ro_max;
    }

    if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_max, using default %d", OB_DEFAULT_HW_MAX);
    return OB_DEFAULT_HW_MAX;
}

int ob_get_min_brightness(bool dbg) {
    int custom_min;
    if (ob_get_prop_int(OB_PROP_DEVMIN, &custom_min) && custom_min > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Using custom devmin brightness: %d", custom_min);
        return custom_min;
    }

    int cached_min;
    if (ob_get_prop_int(OB_PROP_HW_MIN, &cached_min)) {
        if (cached_min > 0) {
            if (dbg) ob_log_d("[DisplayAdaptor] Using cached hw_min: %d", cached_min);
            return cached_min;
        }
        if (dbg) ob_log_d("[DisplayAdaptor] Cached hw_min invalid (%d), forcing to %d.",
                           cached_min, OB_DEFAULT_HW_MIN);
        ob_set_prop_int(OB_PROP_HW_MIN, OB_DEFAULT_HW_MIN);
        return OB_DEFAULT_HW_MIN;
    }

    int val;
    if (ob_read_file_int(OB_PATH_MIN_BRIGHTNESS, &val)) {
        if (val <= 0) {
            if (dbg) ob_log_d("[DisplayAdaptor] Detected hw_min <= 0 (screen off?), falling back to %d.",
                               OB_DEFAULT_HW_MIN);
            val = OB_DEFAULT_HW_MIN;
        }
        if (dbg) ob_log_d("[DisplayAdaptor] Saving hw_min: %d to prop.", val);
        ob_set_prop_int(OB_PROP_HW_MIN, val);
        return val;
    }

    int ro_min;
    if (ob_get_prop_int(OB_PROP_RO_HW_MIN, &ro_min) && ro_min > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_min, using ro prop: %d", ro_min);
        return ro_min;
    }

    if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_min, using default %d", OB_DEFAULT_HW_MIN);
    return OB_DEFAULT_HW_MIN;
}
