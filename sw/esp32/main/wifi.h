/**
 * @file WiFi functionality
 */

#ifndef INTELLILIGHT_WIFI_H
#define INTELLILIGHT_WIFI_H

#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"

#define ACCESS_POINT_SSID "ble-iot-bridge"

/**
 * @brief 
 * @param access_point Choose between access point (AP) or station (STA) mode
 * true to setup ESP32 as an access point for pairing a new device
 * false to connect ESP to a WiFi network using the configured SSID and password
 */
extern void wifi_setup(bool access_point);

/**
 * @brief Check if ESP32 has connected to the WiFi and acquired an IP address
 */
extern bool wifi_network_ready(void);

/**
 * @brief Start TCP server on port 9999
 */
extern void wifi_start_server(void);

#endif
