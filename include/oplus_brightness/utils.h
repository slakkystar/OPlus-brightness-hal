#ifndef OPLUS_BRIGHTNESS_UTILS_H
#define OPLUS_BRIGHTNESS_UTILS_H

#include <stdbool.h>

/* Reads the leading base-10 integer from a sysfs text node.
 * Returns false if the file can't be read or contains no leading digits. */
bool ob_read_file_int(const char *path, int *out);

/*
 * Detects the panel's hardware max/min brightness, in priority order:
 *   1. persist.sys.oplus.brightness.devmax / devmin  (manual override, if > 0)
 *   2. persist.sys.oplus.brightness.hw_max / hw_min   (cached detection)
 *   3. live sysfs read (max_hw_brightness / min_brightness), cached
 *      into hw_max/hw_min on success
 *   4. ro.vendor.display.oplus.brightness.max / min   (vendor build prop)
 *   5. OB_DEFAULT_HW_MAX / OB_DEFAULT_HW_MIN           (last resort)
 */
int ob_get_max_brightness(bool dbg);
int ob_get_min_brightness(bool dbg);

/* Runs `settings get secure panoramic_aod_enable` and returns true iff
 * the result is "1". */
bool ob_is_panoramic_aod_enabled(bool dbg);

#endif /* OPLUS_BRIGHTNESS_UTILS_H */
