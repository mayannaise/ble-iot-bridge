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
 * @brief Flags to remember the current (last commanded) state of the smart bulb
 */
struct smart_bulb_state
{
    bool on_off;            /*!< Current on/off state of the smartbulb */
    uint8_t brightness;     /*!< Current brightness (in percent) of the smartbulb */
    struct hsv_colour hsv;  /*!< Current colour (in HSV) of the smartbulb */
} current_state;

/**
 * @brief Parameters used to scale raw sensor readings to useful data
 */
struct sensor_scale
{
    const uint16_t min_raw;       /*!< Raw minimum value */
    const uint16_t max_raw;       /*!< Raw maximum value */
    const uint8_t  min_scaled;    /*!< Scaled minimum value */
    const uint8_t  max_scaled;    /*!< Scaled maximum value */
};

/**
 * @brief Scale raw sensor readings
 * @param reading Raw sensor value to scale
 * @param scale Sensor scaling data
 * @return Scaled value
 */
uint8_t scale_sensor_reading(const uint16_t reading, const struct sensor_scale scale)
{
    const uint16_t clipped_value = fmin(fmax(reading, scale.min_raw), scale.max_raw);
    const uint16_t range_raw = scale.max_raw - scale.min_raw;
    const uint16_t range_scaled = scale.max_scaled - scale.min_scaled;
    const float factor = ((clipped_value - scale.min_raw) / (float)range_raw);
    return (uint8_t)round((range_scaled * factor) + scale.min_scaled);
}

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
