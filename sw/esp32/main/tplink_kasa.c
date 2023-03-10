/**
 * @file Constants and functions for communicating with TP-Link Kasa IoT smart devices
 */

/* system includes */
#include <string.h>
#include <esp_log.h>

/* local includes */
#include "bluetooth.h"
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

static const char * tplink_kasa_bind = "{\"smartlife.iot.common.cloud\":{\"bind\":{\"err_code\":0}}}";

static const char * tplink_kasa_cloudinfo = \
"{ \
    \"smartlife.iot.common.cloud\": \
    { \
        \"get_info\": \
        { \
            \"username\":\"osjeffreys.uk@gmail.com\", \
            \"server\":\"n-devs.tplinkcloud.com\", \
            \"binded\":1, \
            \"cld_connection\":1, \
            \"illegalType\":0, \
            \"stopConnect\":0, \
            \"tcspStatus\":1, \
            \"fwDlPage\":\"\", \
            \"tcspInfo\":\"\", \
            \"fwNotifyType\":-1, \
            \"err_code\":0 \
        } \
    } \
}";

static const char * tplink_kasa_sysinfo = \
"{ \
    \"system\": \
    { \
        \"get_sysinfo\": \
        { \
        \"sw_ver\":\"1.0.0 Build 000001 Rel.000001\", \
        \"hw_ver\":\"1.0\", \
        \"model\":\"KL130B(UN)\", \
        \"deviceId\":\"80121C1874CF2DEA94DF3127F8DDF7D71DD7112F\", \
        \"oemId\":\"E45F76AD3AF13E60B58D6F68739CD7E5\", \
        \"hwId\":\"1E97141B9F0E939BD8F9679F0B6167C8\", \
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
            \"on_off\":0 \
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

static void tplink_kasa_generate_light_state(cJSON * parent_node, const bool include_on_off)
{
    cJSON_AddItemToObject(parent_node, "mode", cJSON_CreateString("normal"));
    cJSON_AddItemToObject(parent_node, "hue", cJSON_CreateNumber((int)current_state.colour.h));
    cJSON_AddItemToObject(parent_node, "saturation", cJSON_CreateNumber((int)current_state.colour.s));
    cJSON_AddItemToObject(parent_node, "brightness", cJSON_CreateNumber((int)current_state.colour.v));
    cJSON_AddItemToObject(parent_node, "color_temp", cJSON_CreateNumber((int)current_state.temperature));
    if (include_on_off) cJSON_AddItemToObject(parent_node, "on_off", cJSON_CreateNumber((int)current_state.on_off));
    cJSON_AddItemToObject(parent_node, "err_code", cJSON_CreateNumber(0));
}

int tplink_kasa_process_buffer(char * raw_buffer, const int buffer_len, const bool include_header)
{
    int encrypted_len = 0;
    char * json_string = malloc(buffer_len * sizeof(char));

    /* decrypt the received buffer to a JSON string */
    raw_buffer[buffer_len] = 0;
    tplink_kasa_decrypt(raw_buffer, buffer_len, json_string, include_header);

    /* decode JSON message */
    cJSON * rx_json_message = cJSON_Parse(json_string);
    free(json_string);

    if ( rx_json_message == NULL ) {
        ESP_LOGE(log_tag, "Error decoding JSON message");
    } else {
        /* get modules */
        const cJSON * attr_cloudinfo = cJSON_GetObjectItem(rx_json_message, "smartlife.iot.common.cloud");
        const cJSON * attr_light_service = cJSON_GetObjectItem(rx_json_message, "smartlife.iot.smartbulb.lightingservice");

        /* get colour setting from string */
        const cJSON * attr_light_state   = cJSON_GetObjectItem(attr_light_service, "transition_light_state");
        const cJSON * attr_hue           = cJSON_GetObjectItem(attr_light_state,   "hue");
        const cJSON * attr_saturation    = cJSON_GetObjectItem(attr_light_state,   "saturation");
        const cJSON * attr_brightness    = cJSON_GetObjectItem(attr_light_state,   "brightness");
        const cJSON * attr_on_off        = cJSON_GetObjectItem(attr_light_state,   "on_off");

        bool need_to_set_colour = false;

        if (cJSON_IsNumber(attr_hue)) {
            ESP_LOGI(log_tag, "hue %.0f degrees", attr_hue->valuedouble);
            current_state.colour.h = attr_hue->valuedouble;
            need_to_set_colour = true;
        }
        if (cJSON_IsNumber(attr_saturation)) {
            ESP_LOGI(log_tag, "saturation %.0f%%", attr_saturation->valuedouble);
            current_state.colour.s = attr_saturation->valuedouble;
            need_to_set_colour = true;
        }
        if (cJSON_IsNumber(attr_brightness)) {
            ESP_LOGI(log_tag, "brightness %.0f%%", attr_brightness->valuedouble);
            current_state.colour.v = attr_brightness->valuedouble;
            need_to_set_colour = true;
        }

        /* send command to bluetooth bulb to turn on/off if required */
        if (cJSON_IsNumber(attr_on_off)) {
            ESP_LOGI(log_tag, "on/off %.0f", attr_on_off->valuedouble);
            const bool turn_off = attr_on_off->valuedouble == 0;
            if ( turn_off ) {
                bluetooth_turn_bulb_off();
                current_state.on_off = false;
                cJSON * resp = cJSON_CreateObject();
                cJSON_AddItemToObject(resp, "smartlife.iot.smartbulb.lightingservice", cJSON_CreateObject());
                cJSON * light_service = cJSON_GetObjectItem(resp, "smartlife.iot.smartbulb.lightingservice");
                cJSON_AddItemToObject(light_service, "transition_light_state", cJSON_CreateObject());
                tplink_kasa_generate_light_state(cJSON_GetObjectItem(light_service, "transition_light_state"), true);
                encrypted_len = tplink_kasa_encrypt(resp, raw_buffer, include_header);
                cJSON_Delete(resp);
                need_to_set_colour = false;
            } else {
                /* this will turn the bulb back on */
                need_to_set_colour = true;
            }
        }

        /* send command to bluetooth bulb to set the colour */
        if (need_to_set_colour) {
            const struct rgb_colour rgb = colours_hsv_to_rgb(current_state.colour);
            ESP_LOGI(log_tag, "set bulb HSV %.0f,%.0f,%.0f", current_state.colour.h, current_state.colour.s, current_state.colour.v);
            ESP_LOGI(log_tag, "set bulb RGB %d,%d,%d", rgb.r, rgb.g, rgb.b);
            bluetooth_set_bulb_colour(colours_hsv_to_rgb(current_state.colour));
            current_state.on_off = true;
            cJSON * resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "smartlife.iot.smartbulb.lightingservice", cJSON_CreateObject());
            cJSON * light_service = cJSON_GetObjectItem(resp, "smartlife.iot.smartbulb.lightingservice");
            cJSON_AddItemToObject(light_service, "transition_light_state", cJSON_CreateObject());
            tplink_kasa_generate_light_state(cJSON_GetObjectItem(light_service, "transition_light_state"), true);
            encrypted_len = tplink_kasa_encrypt(resp, raw_buffer, include_header);
            cJSON_Delete(resp);
        }

        /* check for cloud info request */
        if ( cJSON_HasObjectItem(attr_cloudinfo, "get_info") ) {
            cJSON * response_template = cJSON_Parse(tplink_kasa_cloudinfo);
            encrypted_len = tplink_kasa_encrypt(response_template, raw_buffer, include_header);
            cJSON_Delete(response_template);
        }

        /* check for cloud bind request */
        if ( cJSON_HasObjectItem(attr_cloudinfo, "bind") ) {
            cJSON * response_template = cJSON_Parse(tplink_kasa_bind);
            encrypted_len = tplink_kasa_encrypt(response_template, raw_buffer, include_header);
            cJSON_Delete(response_template);
        }

        /* check for system info request */
        const cJSON * attr_system = cJSON_GetObjectItem(rx_json_message, "system");
        if ( cJSON_HasObjectItem(attr_system, "get_sysinfo") ) {
            ESP_LOGI(log_tag, "System information requested");
            bluetooth_request_bulb_state();
            /* generate the JSON response from the template */
            cJSON * response_template = cJSON_Parse(tplink_kasa_sysinfo);
            if ( response_template == NULL ) {
                ESP_LOGE(log_tag, "Error generating system info JSON");
            } else {
                cJSON * resp_system = cJSON_GetObjectItem(response_template, "system");
                cJSON * resp_sysinfo = cJSON_GetObjectItem(resp_system, "get_sysinfo");
                cJSON * resp_light_state = cJSON_GetObjectItem(resp_sysinfo, "light_state");
                cJSON * resp_on_off = cJSON_GetObjectItem(resp_light_state, "on_off");
                if (current_state.on_off) {
                    tplink_kasa_generate_light_state(resp_light_state, false);
                } else {
                    cJSON_AddItemToObject(resp_light_state, "dft_on_state", cJSON_CreateObject());
                    cJSON * resp_dft_state = cJSON_GetObjectItem(resp_light_state, "dft_on_state");
                    tplink_kasa_generate_light_state(resp_dft_state, false);
                }
                cJSON_SetNumberValue(resp_on_off, (int)current_state.on_off);
                encrypted_len = tplink_kasa_encrypt(response_template, raw_buffer, include_header);
                cJSON_Delete(response_template);
            }
        }
        /* tidy up */
        cJSON_Delete(rx_json_message);
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

    ESP_LOGD(log_tag, "Encrypted payload (%d bytes): %s", encrypted_len, encrypted_payload);
    ESP_LOGD(log_tag, "Decrypted payload (%d bytes): %s", header.payload_length, decrypted_payload);

    return header.payload_length;
}

int tplink_kasa_encrypt(const cJSON * json, char * encrypted_payload, const bool include_header)
{
    /* autokey cypher key value */
    char key = cipher_key;

    /* convert JSON object to string and allocate on the HEAP (must free memory when finished) */
    char * payload = cJSON_PrintUnformatted(json);

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
    for (int i = 0; i < strlen(payload); i++)
    {
        key = encrypted_payload[i + header_len] = payload[i] ^ key;
    }

    const int encrypted_len = strlen(payload) + header_len;

    ESP_LOGD(log_tag, "Decrypted payload (%d bytes): %s", header.payload_length, payload);
    ESP_LOGD(log_tag, "Encrypted payload (%d bytes): %s", encrypted_len, encrypted_payload);

    free(payload);

    return encrypted_len;
}
