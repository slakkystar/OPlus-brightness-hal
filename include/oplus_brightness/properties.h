#ifndef OPLUS_BRIGHTNESS_PROPERTIES_H
#define OPLUS_BRIGHTNESS_PROPERTIES_H

#include <stdbool.h>
#include <stddef.h>

/* System property helpers, built on libcutils' property_get/property_set.
 *
 * ob_get_prop:      copies the property value into `out` (size out_size).
 *                    Returns false if unset/empty.
 * ob_get_prop_int:   parses the property value as a base-10 integer.
 *                    Returns false if unset or not a valid integer.
 * ob_get_prop_bool:  true iff the property value is exactly "true".
 * ob_set_prop:       sets a property. Returns false on failure.
 */
bool ob_get_prop(const char *key, char *out, size_t out_size);
bool ob_get_prop_int(const char *key, int *out);
bool ob_get_prop_bool(const char *key);
bool ob_set_prop(const char *key, const char *val);
bool ob_set_prop_int(const char *key, int val);

#endif /* OPLUS_BRIGHTNESS_PROPERTIES_H */
