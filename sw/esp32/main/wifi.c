/**
 * @file WiFi functionality
 */

/* system includes */
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

/* local includes */
#include "wifi.h"


static const char *log_tag = "wifi";
static bool wifi_connected = false;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        wifi_connected = true;
        ESP_LOGI(log_tag, "ESP acquired IP address:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static esp_err_t configure_nvs_flash(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    return ret;
}

bool wifi_network_ready(void)
{
    return wifi_connected;
}

void wifi_connect(void)
{
    ESP_ERROR_CHECK(configure_nvs_flash());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool wifi_tcp_transfer(const char *payload, const int length)
{
    char rx_buffer[128];
    struct sockaddr_in dest_addr;
    int err;
    int sock;

    if (!wifi_connected) {
        return false;
    }

    inet_pton(AF_INET, CONFIG_SMARTBULB_IP_ADDRESS, &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9999);

    /* create socket */
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(log_tag, "Unable to create socket: errno %d", errno);
        return false;
    }

    /* connect to TCP server */
    err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(log_tag, "Socket unable to connect to %s: errno %d", CONFIG_SMARTBULB_IP_ADDRESS, errno);
        return false;
    }

    /* send payload */
    err = send(sock, payload, length, 0);
    if (err < 0) {
        ESP_LOGE(log_tag, "TCP send error: %d", errno);
        return false;
    }

    /* check for any returned data */
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE(log_tag, "TCP recv error: %d", errno);
        return false;
    }

    /* shutdown socket */
    shutdown(sock, 0);
    close(sock);
    return true;
}
