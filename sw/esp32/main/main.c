/**
 * @file Main application code
 */
 
/* system includes */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* ESP-IDF includes */
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/i2c.h"
#include "driver/rtc_io.h"

/* local includes */
#include "bluetooth.h"
#include "wifi.h"


/**
 * @brief Application main entry point
 */
void app_main(void)
{
    /* in pairing mode, the ESP32 is configured as an access point */
    /* otherwise connect the ESP32 to an access point */
    bool bulb_needs_pairing = false;
    wifi_setup(bulb_needs_pairing);

    /* start bluetooth and connect to BLE smartbulb */
    bluetooth_start();
}
