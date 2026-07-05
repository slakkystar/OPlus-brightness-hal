#include <math.h>

#include "oplus_brightness/constants.h"
#include "oplus_brightness/scaling.h"

int ob_scale_brightness_curved(int val, int hw_min, int hw_max,
                                int input_min, int input_max) {
    if (val <= input_min) {
        return hw_min;
    }
    if (val >= input_max) {
        return hw_max;
    }

    float range_input = (float)(input_max - input_min);
    float range_hw = (float)(hw_max - hw_min);
    float ratio = (float)(val - input_min) / range_input;

    /* Standard gamma 2.2 approximation (perceptual -> linear). */
    float curve = powf(ratio, OB_GAMMA);

    return (int)lroundf((float)hw_min + curve * range_hw);
}
