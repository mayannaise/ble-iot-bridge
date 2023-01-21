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
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
