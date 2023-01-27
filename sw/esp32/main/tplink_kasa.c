/**
 * @file Constants and functions for communicating with TP-Link Kasa IoT smart devices
 */

/* system includes */
#include <string.h>
#include <esp_log.h>

/* local includes */
#include "bluetooth.h"
#include "colours.h"
#include "cjson/cJSON.h"
#include "tplink_kasa.h"
#include "wifi.h"

static const char *log_tag = "tplink-kasa";

union payload_header
{
    char bytes[4];
    uint32_t payload_length;
};

const char cipher_key = 171;

static const char * tplink_kasa_sysinfo = "{ \
  \"system\": \
  { \
    \"get_sysinfo\": \
    { \
      \"sw_ver\":\"1.0.0 Build 000001 Rel.000001\", \
      \"hw_ver\":\"1.0\", \
      \"model\":\"KL130B(UN)\", \
      \"deviceId\":\"80121C1874CF2DEA94DF3127F8DDF7D71DD7112F\", \
      \"oemId\":\"E45F76AD3AF13E60B58D6F68739CD7E5\", \
      \"hwId\":\"1E97141B9F0E939BD8F9679F0B6167C9\", \
      \"rssi\":-71, \
      \"latitude_i\":0, \
      \"longitude_i\":0, \
      \"alias\":\"Back Light\", \
      \"status\":\"new\", \
      \"description\":\"WiFi BLE Smart Bulb Bridge\", \
      \"mic_type\":\"IOT.SMARTBULB\", \
      \"mic_mac\":\"C0C9E3AD7C1D\", \
      \"dev_state\":\"normal\", \
      \"is_factory\":false, \
      \"disco_ver\":\"1.0\", \
      \"ctrl_protocols\":  \
      { \
        \"name\":\"Linkie\", \
        \"version\":\"1.0\" \
      }, \
      \"active_mode\":\"none\", \
      \"is_dimmable\":1, \
      \"is_color\":1, \
      \"is_variable_color_temp\":1, \
      \"light_state\": \
      { \
        \"on_off\":0, \
        \"dft_on_state\": \
        { \
          \"mode\":\"normal\", \
          \"hue\":0, \
          \"saturation\":0, \
          \"color_temp\":2700, \
          \"brightness\":75 \
        } \
      }, \
      \"preferred_state\":[ \
        { \
          \"index\":0, \
          \"hue\":0, \
          \"saturation\":0, \
          \"color_temp\":2700, \
          \"brightness\":50 \
        }, \
        { \
          \"index\":1, \
          \"hue\":0, \
          \"saturation\":100, \
          \"color_temp\":0, \
          \"brightness\":100 \
        }, \
        { \
          \"index\":2, \
          \"hue\":120, \
          \"saturation\":100, \
          \"color_temp\":0, \
          \"brightness\":100 \
        }, \
        { \
          \"index\":3, \
          \"hue\":240, \
          \"saturation\":100, \
          \"color_temp\":0, \
          \"brightness\":100 \
        } \
      ], \
      \"err_code\":0 \
    } \
  } \
}";

/* record the last known state of the bulb so we can update a single value at a time if required */
/* default to white (0 degrees, 0% saturation, 100% brightness) */
struct hsv_colour current_colour = { .h = 0, .s = 0, .v = 100 };

int tplink_kasa_process_buffer(char * raw_buffer, const int buffer_len, const bool include_header)
{
    int encrypted_len = 0;
    char json_string[200];

    /* decrypt the received buffer to a JSON string */
    raw_buffer[buffer_len] = 0;
    const int decrypted_len = tplink_kasa_decrypt(raw_buffer, buffer_len, json_string, include_header);
    ESP_LOGI(log_tag, "Encrypted payload (%d bytes): %s", buffer_len, raw_buffer);
    ESP_LOGI(log_tag, "Decrypted payload (%d bytes): %s", decrypted_len, json_string);

    /* decode JSON message */
    cJSON *json_message = cJSON_Parse(json_string);
    if ( json_message == NULL ) {
        ESP_LOGE(log_tag, "Error decoding JSON message");
    } else {
        /* get system info request from string */
        const cJSON * attr_system = cJSON_GetObjectItem(json_message, "system");
        if ( cJSON_HasObjectItem(attr_system, "get_sysinfo") ) {
            ESP_LOGI(log_tag, "System information requested");
            encrypted_len = tplink_kasa_encrypt(tplink_kasa_sysinfo, raw_buffer, include_header);
        }

        /* get colour setting from string */
        const cJSON * attr_light_service = cJSON_GetObjectItem(json_message,       "smartlife.iot.smartbulb.lightingservice");
        const cJSON * attr_light_state   = cJSON_GetObjectItem(attr_light_service, "transition_light_state");
        const cJSON * attr_hue           = cJSON_GetObjectItem(attr_light_state,   "hue");
        const cJSON * attr_saturation    = cJSON_GetObjectItem(attr_light_state,   "saturation");
        const cJSON * attr_brightness    = cJSON_GetObjectItem(attr_light_state,   "brightness");
        const cJSON * attr_on_off        = cJSON_GetObjectItem(attr_light_state,   "on_off");

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

    return encrypted_len;
}

int tplink_kasa_decrypt(const char * encrypted_payload, const int encrypted_len, char * decrypted_payload, const bool include_header)
{
    /* if the encrypted length is less than the length of a header then it cannot be valid */
    if (encrypted_len <= sizeof(union payload_header))
    {
        decrypted_payload[0] = 0;
        return 0;
    }

    /* autokey cypher key value */
    char key = cipher_key;

    /* the first 4 bytes in the encrypted data define the length of the payload, encoded in big endian */
    /* since ESP32 is little endian, need to swap the endianness */
    union payload_header header;
    header.bytes[3] = encrypted_payload[0];
    header.bytes[2] = encrypted_payload[1];
    header.bytes[1] = encrypted_payload[2];
    header.bytes[0] = encrypted_payload[3];
    int header_len = sizeof(header);

    /* if payload length comes out bigger than the entire data packet then it is probably not valid */
    /* but we will truncate it and decode what we can just in case (maybe we only got half a packet) */
    if (header.payload_length > encrypted_len - header_len)
    {
        header_len = 0;
        header.payload_length = encrypted_len;
    }

    /* XOR each byte with the previous encypted byte or 171 for the first byte */
    for (int i = 0; i < header.payload_length; i++)
    {
        decrypted_payload[i] = encrypted_payload[i + header_len] ^ key;
        key = encrypted_payload[i + header_len];
    }

    /* stick a null on the end to terminate the string */
    decrypted_payload[header.payload_length] = '\0';
    return header.payload_length;
}

int tplink_kasa_encrypt(const char * payload, char * encrypted_payload, const bool include_header)
{
    /* autokey cypher key value */
    char key = cipher_key;

    /* the first 4 bytes in the encrypted data define the length of the payload, encoded in big endian */
    /* since ESP32 is little endian, need to swap the endianness */
    union payload_header header;
    header.payload_length = strlen(payload);
    encrypted_payload[0] = header.bytes[3];
    encrypted_payload[1] = header.bytes[2];
    encrypted_payload[2] = header.bytes[1];
    encrypted_payload[3] = header.bytes[0];

    /* header length (may or may not be present) */
    const int header_len = include_header ? sizeof(header) : 0;
    
    /* XOR each byte with the previous encypted byte or 171 for the first byte */
    for (int i = 0; i < header.payload_length; i++)
    {
        key = encrypted_payload[i + header_len] = payload[i] ^ key;
    }

    return header.payload_length + header_len;
}
