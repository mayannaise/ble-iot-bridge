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
#include "freertos/FreeRTOS.h"

#include "colours.h"


/**
 * @brief Set the colour of the BLE smart bulb
 * @param rgb New colour
 */
extern bool bluetooth_set_bulb_colour(const struct rgb_colour rgb);

/**
 * @brief Turn off BLE smart bulb
 */
extern bool bluetooth_turn_bulb_off();

/**
 * @brief Configure bluetooth on the ESP
 */
extern void bluetooth_start(void);

#endif
