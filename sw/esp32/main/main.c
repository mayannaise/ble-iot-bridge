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
#include "tplink_kasa.h"
#include "colours.h"
#include "wifi.h"


/**
 * @brief Flags to remember the current state of the smart bulb
 */
struct smart_bulb_state
{
    bool    on_off;         /*!< Current on/off state of the smartbulb */
    uint8_t brightness;     /*!< Current brightness (in percent) of the smartbulb */
    struct  hsv_colour hsv; /*!< Current colour (in HSV) of the smartbulb */
} current_state;

/**
 * @brief Application main entry point
 */
void app_main(void)
{
    /* application constants */
    const char *log_tag = "ble-iot-bridge";

    /* connect to the configured WiFi network */
    ESP_LOGI(log_tag, "Conecting to WiFi");
    wifi_connect();

    /* wait till network is ready */
    while ( !wifi_network_ready() ) {
        vTaskDelay(500 / portTICK_RATE_MS);
    }

    /* main loop */
    ESP_LOGI(log_tag, "Entering main loop");
    while (true)
    {
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}
