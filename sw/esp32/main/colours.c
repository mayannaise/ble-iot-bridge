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
        hsv.s = (rgb_delta / rgb_max) * 100;
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
    
    double C = (hsv.v / 100) * (hsv.s / 100);
    double X = C * (1 - abs(fmod((hsv.h / 60), 2) - 1));
    double m = (hsv.v / 100) - C;

    if (0 <= hsv.h && hsv.h < 60)
        rgb = (struct rgb_colour){.r = C, .g = X, .b = 0};
    else if (60 <= hsv.h && hsv.h < 120)
        rgb = (struct rgb_colour){.r = X, .g = C, .b = 0};
    else if (120 <= hsv.h && hsv.h < 180)
        rgb = (struct rgb_colour){.r = 0, .g = C, .b = X};
    else if (180 <= hsv.h && hsv.h < 240)
        rgb = (struct rgb_colour){.r = 0, .g = X, .b = C};
    else if (240 <= hsv.h && hsv.h < 300)
        rgb = (struct rgb_colour){.r = X, .g = 0, .b = C};
    else if (300 <= hsv.h && hsv.h < 360)
        rgb = (struct rgb_colour){.r = C, .g = 0, .b = X};
    else
        rgb = (struct rgb_colour){.r = 0.0, .g = 0.0, .b = 0.0};

    rgb.r = (rgb.r + m) * 255;
    rgb.g = (rgb.g + m) * 255;
    rgb.b = (rgb.b + m) * 255;

    return rgb;
}
