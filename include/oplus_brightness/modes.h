#ifndef OPLUS_BRIGHTNESS_MODES_H
#define OPLUS_BRIGHTNESS_MODES_H

#include <stdbool.h>

/* Reads persist.sys.oplus.brightness.debug. */
bool ob_dbg_on(void);

/* Entry point: runs the OS15+ brightness poll loop. Never returns. */
void ob_run(void);

#endif /* OPLUS_BRIGHTNESS_MODES_H */
