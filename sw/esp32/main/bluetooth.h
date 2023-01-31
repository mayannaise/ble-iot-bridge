/**
 * @file Bluetooth Low Energy (BLE) functionality
 */

#ifndef INTELLILIGHT_BLUETOOTH_H
#define INTELLILIGHT_BLUETOOTH_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include <freertos/task.h>
#include "freertos/FreeRTOS.h"

#include "colours.h"

/**
 * @brief structure to remember the last state of the bulb
 */
struct light_state
{
    struct hsv_colour colour;
    bool on_off;
    int temperature;
    bool up_to_date;
};

extern struct light_state current_state;

/**
 * @brief Set the colour of the BLE smart bulb
 * @param rgb New colour
 */
extern bool bluetooth_set_bulb_colour(const struct rgb_colour rgb);

/**
 * @brief Request a reading of the colour of the BLE smart bulb
 */
extern void bluetooth_request_bulb_state();

/**
 * @brief Turn off BLE smart bulb
 */
extern bool bluetooth_turn_bulb_off();

/**
 * @brief Configure bluetooth on the ESP
 */
extern void bluetooth_start(void);

#endif
