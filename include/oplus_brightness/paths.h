/*
 * paths.h — sysfs node paths and system-property keys.
 *
 * OS15+ only: the legacy OS14 "DisplayPanel" bridge mode has been
 * removed entirely from this fork.
 */
#ifndef OPLUS_BRIGHTNESS_PATHS_H
#define OPLUS_BRIGHTNESS_PATHS_H

/* ---------------------------------------------------------------------
 * sysfs backlight nodes
 * ------------------------------------------------------------------- */
#define OB_PATH_BRIGHTNESS      "/sys/class/leds/lcd-backlight/brightness"
#define OB_PATH_MIN_BRIGHTNESS  "/sys/class/leds/lcd-backlight/min_brightness"
#define OB_PATH_MAX_BRIGHTNESS  "/sys/class/leds/lcd-backlight/max_hw_brightness"

/* ---------------------------------------------------------------------
 * Framework-owned properties (not part of our namespace, read-only to us)
 * ------------------------------------------------------------------- */
#define OB_PROP_SCREEN_BRIGHTNESS "debug.tracing.screen_brightness"
#define OB_PROP_SCREEN_STATE      "debug.tracing.screen_state"

/* Live IR-detected multi-brightness range, written by the ROM itself. */
#define OB_PROP_SYS_MULTIBRIGHTNESS_MAX "sys.oplus.multibrightness"
#define OB_PROP_SYS_MULTIBRIGHTNESS_MIN "sys.oplus.multibrightness.min"

/* ---------------------------------------------------------------------
 * RO build/vendor properties — authoritative panel hardware bounds
 * ------------------------------------------------------------------- */
#define OB_PROP_RO_HW_MAX "ro.vendor.display.oplus.brightness.max"
#define OB_PROP_RO_HW_MIN "ro.vendor.display.oplus.brightness.min"

/* ---------------------------------------------------------------------
 * Persisted (persist.*) configuration properties
 * ------------------------------------------------------------------- */
#define OB_PROP_DEBUG      "persist.sys.oplus.brightness.debug"
#define OB_PROP_ISFLOAT    "persist.sys.oplus.brightness.isfloat"

/* Kept only for compatibility with older configs / scripts. The adaptor
 * always renders with the Curved gamma-2.2 curve regardless of this
 * value — Linear (1) and Custom (2) modes have been removed. */
#define OB_PROP_MODE       "persist.sys.oplus.brightness.mode"

#define OB_PROP_DISPLAY_TYPE "persist.sys.oplus.brightness.display.type"

/* Manual override of the HW range used for scaling (higher priority than
 * ro.vendor.display.oplus.brightness.*). */
#define OB_PROP_DEVMAX "persist.sys.oplus.brightness.devmax"
#define OB_PROP_DEVMIN "persist.sys.oplus.brightness.devmin"

/* Cache of the detected HW range, written by the service itself. */
#define OB_PROP_HW_MAX "persist.sys.oplus.brightness.hw_max"
#define OB_PROP_HW_MIN "persist.sys.oplus.brightness.hw_min"

/* Cache of the detected logical (input) range, written by the service
 * itself once IR / multibrightness locks it in. */
#define OB_PROP_RANGE_MAX "persist.sys.oplus.brightness.range.max"
#define OB_PROP_RANGE_MIN "persist.sys.oplus.brightness.range.min"

/* Lux AOD (always-on-display auto brightness) behaviour. */
#define OB_PROP_LUX_AOD       "persist.sys.oplus.brightness.lux_aod"
#define OB_PROP_LUX_AOD_VALUE "persist.sys.oplus.brightness.lux_aod.value"

#endif /* OPLUS_BRIGHTNESS_PATHS_H */
