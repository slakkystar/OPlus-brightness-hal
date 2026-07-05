#ifndef OPLUS_BRIGHTNESS_SCALING_H
#define OPLUS_BRIGHTNESS_SCALING_H

/*
 * Maps a logical (framework/input) brightness value onto the panel's
 * hardware brightness range using a perceptual gamma-2.2 curve.
 *
 * This is intentionally the ONLY scaling curve supported: the original
 * project's Linear and Custom (75%->255) modes have been removed, since
 * Curved gamma-2.2 is the smoothest and is always what we want here.
 *
 * val:        raw logical brightness value read from the framework
 * hw_min/max: physical panel brightness bounds
 * input_min/max: logical brightness bounds the value is expressed in
 */
int ob_scale_brightness_curved(int val, int hw_min, int hw_max,
                                int input_min, int input_max);

#endif /* OPLUS_BRIGHTNESS_SCALING_H */
