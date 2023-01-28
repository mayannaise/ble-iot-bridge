/**
 * @file Generic functions to manipulate colours
 */

#ifndef INTELLILIGHT_COLOURS_H
#define INTELLILIGHT_COLOURS_H

/**
 * @brief RGB definition
 */
struct rgb_colour
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

/**
 * @brief HSV definition
 */
struct hsv_colour
{
    float h;
    float s;
    float v;
    float t;
};

/**
 * @brief Convert RGB to HSV
 * @param rgb RGB value to convert
 * @return HSV representation of the input RGB value
 */
extern struct hsv_colour colours_rgb_to_hsv(struct rgb_colour rgb);

/**
 * @brief Convert HSV to RGB
 * @param hsv HSV value to convert
 * @return RGB representation of the input HSV value
 */
extern struct rgb_colour colours_hsv_to_rgb(struct hsv_colour hsv);

#endif
