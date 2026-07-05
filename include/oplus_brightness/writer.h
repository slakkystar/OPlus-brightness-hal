#ifndef OPLUS_BRIGHTNESS_WRITER_H
#define OPLUS_BRIGHTNESS_WRITER_H

#include <stdbool.h>

/* Writes `val` to the already-open brightness fd, but only if it differs
 * from *last_val. Updates *last_val on success. */
void ob_write_brightness(int fd, int val, int *last_val, bool dbg);

#endif /* OPLUS_BRIGHTNESS_WRITER_H */
