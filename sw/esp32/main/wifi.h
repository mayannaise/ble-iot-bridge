/**
 * @file WiFi functionality
 */

#ifndef INTELLILIGHT_WIFI_H
#define INTELLILIGHT_WIFI_H

#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"


/**
 * @brief Connect to a WiFi network using the configured SSID and password
 */
extern void wifi_connect(void);

/**
 * @brief Check if ESP32 has connected to the WiFi and acquired an IP address
 */
extern bool wifi_network_ready(void);

/**
 * @brief Start TCP server on port 9999
 */
extern void wifi_start_server(void);

#endif
