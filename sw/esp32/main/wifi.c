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
#include "bluetooth.h"
#include "cjson/cJSON.h"
#include "wifi.h"
#include "tplink_kasa.h"

/* constants */
static const char *log_tag = "wifi";
static const uint32_t port = 9999;
static const uint8_t mac_address[] = {0xC0, 0xC9, 0xE3, 0xAD, 0x7C, 0x1C};

/* fag to indicate if the WiFi has connected yet */
static bool wifi_connected = false;

/* record the last known state of the bulb so we can update a single value at a time if required */
/* default to white (0 degrees, 0% saturation, 100% brightness) */
struct hsv_colour current_colour = { .h = 0, .s = 0, .v = 100 };


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
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_connected = true;
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(log_tag, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
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

void wifi_setup(bool access_point)
{
    ESP_ERROR_CHECK(configure_nvs_flash());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    if ( access_point )
    {
        esp_netif_t *netif = esp_netif_create_default_wifi_ap();
        assert(netif);

        wifi_config_t wifi_config = {
            .ap = {
                .ssid = ACCESS_POINT_SSID,
                .ssid_len = strlen(ACCESS_POINT_SSID),
                .channel = 1,
                .max_connection = 1,
                .authmode = WIFI_AUTH_OPEN
            },
        };

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, &mac_address[0]));
    }
    else
    {
        esp_netif_t *netif = esp_netif_create_default_wifi_sta();
        assert(netif);

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
        ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_STA, &mac_address[0]));
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void process_buffer(const int sock)
{
    int buffer_len;
    char raw_buffer[200];
    char json_string[200];

    do {
        buffer_len = recv(sock, raw_buffer, sizeof(raw_buffer) - 1, 0);
        if (buffer_len < 0) {
            ESP_LOGE(log_tag, "Error occurred during receiving: errno %d", errno);
        } else if (buffer_len == 0) {
            ESP_LOGW(log_tag, "Connection closed");
        } else {
            /* decrypt the received buffer to a JSON string */
            raw_buffer[buffer_len] = 0;
            tplink_kasa_decrypt(raw_buffer, json_string);
            ESP_LOGI(log_tag, "Encrypted payload (%d bytes): %s", buffer_len, raw_buffer);
            ESP_LOGI(log_tag, "Decrypted payload (%d bytes): %s", strlen(json_string), json_string);

            /* decode JSON message */
            cJSON *json_message = cJSON_Parse(json_string);
            if ( json_message == NULL ) {
                ESP_LOGE(log_tag, "Error decoding JSON message");
            } else {
                /* get system info request from string */
                const cJSON * attr_system = cJSON_GetObjectItem(json_message, "system");
                if ( cJSON_HasObjectItem(attr_system, "get_sysinfo") ) {
                    ESP_LOGI(log_tag, "get sysinfo request");
                    //buffer_len = tplink_kasa_encrypt(tplink_kasa_sysinfo, raw_buffer);
                    buffer_len = tplink_kasa_encrypt("{\"system\":{\"get_sysinfo\":{\"err_code\":0}}}", raw_buffer);
                }

                /* get colour setting from string */
                const cJSON * attr_light_service = cJSON_GetObjectItem(json_message, "smartlife.iot.smartbulb.lightingservice");
                const cJSON * attr_light_state = cJSON_GetObjectItem(attr_light_service, "transition_light_state");
                const cJSON * attr_hue = cJSON_GetObjectItem(attr_light_state, "hue");
                const cJSON * attr_saturation = cJSON_GetObjectItem(attr_light_state, "saturation");
                const cJSON * attr_brightness = cJSON_GetObjectItem(attr_light_state, "brightness");
                const cJSON * attr_on_off = cJSON_GetObjectItem(attr_light_state, "on_off");

                bool need_to_set_colour = false;

                if (cJSON_IsNumber(attr_hue)) {
                    ESP_LOGI(log_tag, "hue %.0f degrees", attr_hue->valuedouble);
                    current_colour.h = attr_hue->valuedouble;
                    need_to_set_colour = true;
                }
                if (cJSON_IsNumber(attr_saturation)) {
                    ESP_LOGI(log_tag, "saturation %.0f%%", attr_saturation->valuedouble);
                    current_colour.s = attr_saturation->valuedouble;
                    need_to_set_colour = true;
                }
                if (cJSON_IsNumber(attr_brightness)) {
                    ESP_LOGI(log_tag, "brightness %.0f%%", attr_brightness->valuedouble);
                    current_colour.v = attr_brightness->valuedouble;
                    need_to_set_colour = true;
                }

                /* send command to bluetooth bulb to turn on/off if required */
                if (cJSON_IsNumber(attr_on_off)) {
                    ESP_LOGI(log_tag, "on/off %.0f", attr_on_off->valuedouble);
                    const bool turn_off = attr_on_off->valuedouble == 0;
                    if ( turn_off ) {
                        bluetooth_turn_bulb_off();
                        need_to_set_colour = false;
                    } else {
                        /* this will turn the bulb back on */
                        need_to_set_colour = true;
                    }
                }

                /* send command to bluetooth bulb to set the colour */
                if (need_to_set_colour) {
                    bluetooth_set_bulb_colour(colours_hsv_to_rgb(current_colour));
                }

                /* tidy up */
                cJSON_Delete(json_message);
            }

            /* return response okay message */
            ESP_LOGI(log_tag, "Sending response (%d bytes)", buffer_len);
            int to_write = buffer_len;
            while (to_write > 0)
            {
                int written = send(sock, raw_buffer + (buffer_len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(log_tag, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }
        }
    } while (buffer_len > 0);
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = 5;
    int keepInterval = 5;
    int keepCount = 3;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(port);
        ip_protocol = IPPROTO_IP;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(log_tag, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(log_tag, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(log_tag, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(log_tag, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(log_tag, "Socket bound, port %d", port);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(log_tag, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(log_tag, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(log_tag, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 5, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 5, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, 3, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(log_tag, "Socket accepted ip address: %s", addr_str);

        process_buffer(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void wifi_start_server(void)
{
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
}
