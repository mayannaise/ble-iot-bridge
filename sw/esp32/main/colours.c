/**
 * @file Generic functions to manipulate colours
 */
 
/* local includes */
#include "colours.h"


struct hsv_colour colours_rgb_to_hsv(struct rgb_colour rgb)
{
    struct hsv_colour hsv;
    
    /* calculate HSV "value" component (0-100% range) */
    const unsigned char rgb_min = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    const unsigned char rgb_max = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);
    const float rgb_delta = rgb_max - rgb_min;
    hsv.v = rgb_max / 2.55;

    /* calculate HSV "saturation" component (0-100% range) */
    if (hsv.v == 0) {
        hsv.s = 0;
    } else {
        hsv.s = rgb_delta / rgb_max;
    }

    /* calculate HSV "hue" component (0-360 degrees) */
    if (rgb_delta == 0) {
        hsv.h = 0;
    } else if (rgb_max == rgb.r) {
        hsv.h = 0 + ((rgb.g - rgb.b) / rgb_delta);
    } else if (rgb_max == rgb.g) {
        hsv.h = 2 + ((rgb.b - rgb.r) / rgb_delta);
    } else if (rgb_max == rgb.b) {
        hsv.h = 4 + ((rgb.r - rgb.g) / rgb_delta);
    }
    hsv.h = hsv.h * 60;
    if (hsv.h < 0) hsv.h += 360;

    return hsv;
}
