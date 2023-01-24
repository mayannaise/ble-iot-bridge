/**
 * @file Generic functions to manipulate colours
 */

/* system includes */
#include <math.h>

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

struct rgb_colour colours_hsv_to_rgb(struct hsv_colour hsv)
{
    struct rgb_colour rgb = {};
    double P, Q, T, fract;

    (hsv.h == 360.0) ? (hsv.h = 0.0) : (hsv.h /= 60.0);
    fract = hsv.h - floor(hsv.h);

    P = hsv.v * (1.0 - hsv.s);
    Q = hsv.v * (1.0 - hsv.s * fract);
    T = hsv.v * (1.0 - hsv.s * (1.0 - fract));

    if (0.0 <= hsv.h && hsv.h < 1.0)
        rgb = (struct rgb_colour){.r = hsv.v, .g = T, .b = P};
    else if (1.0 <= hsv.h && hsv.h < 2.0)
        rgb = (struct rgb_colour){.r = Q, .g = hsv.v, .b = P};
    else if (2.0 <= hsv.h && hsv.h < 3.0)
        rgb = (struct rgb_colour){.r = P, .g = hsv.v, .b = T};
    else if (3.0 <= hsv.h && hsv.h < 4.0)
        rgb = (struct rgb_colour){.r = P, .g = Q, .b = hsv.v};
    else if (4.0 <= hsv.h && hsv.h < 5.0)
        rgb = (struct rgb_colour){.r = T, .g = P, .b = hsv.v};
    else if (5.0 <= hsv.h && hsv.h < 6.0)
        rgb = (struct rgb_colour){.r = hsv.v, .g = P, .b = Q};
    else
        rgb = (struct rgb_colour){.r = 0.0, .g = 0.0, .b = 0.0};

    return rgb;
}
