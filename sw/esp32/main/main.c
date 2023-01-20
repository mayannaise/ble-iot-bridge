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
#include "tplink_kasa.h"
#include "colours.h"
#include "wifi.h"


/**
 * @brief Flags to remember the current state of the smart bulb
 */
struct smart_bulb_state
{
    bool    on_off;            /*!< Current on/off state of the smartbulb */
    uint8_t brightness;        /*!< Current brightness (in percent) of the smartbulb */
    struct  rgb_colour colour; /*!< Current colour (in RGB) of the smartbulb */
} current_state;

/**
 * @brief Application main entry point
 */
void app_main(void)
{
    /* application constants */
    const char *log_tag = "ble-iot-bridge";

    /* connect to the configured WiFi network */
    ESP_LOGI(log_tag, "Conecting to WiFi network");
    wifi_connect();

    /* wait till network is ready and start TCP server */
    while ( !wifi_network_ready() )
    {
        vTaskDelay(500 / portTICK_RATE_MS);
    }
    ESP_LOGI(log_tag, "Starting WiFi smartbulb emulator (TCP server port 9999)");
    wifi_start_server();

    /* start bluetooth and connect to BLE smartbulb */
    bluetooth_start();

    /* main loop to bridge WiFi and BLE */
    while (true)
    {
        current_state.colour.r = rand() % 256;
        current_state.colour.g = rand() % 256;
        current_state.colour.b = rand() % 256;
        bluetooth_set_bulb_colour(current_state.colour);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
