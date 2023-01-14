/**
 * @file Constants and functions for communicating with TP-Link Kasa IoT smart devices
 */

/* system includes */
#include <string.h>
#include <esp_log.h>

/* local includes */
#include "tplink_kasa.h"
#include "wifi.h"


const char * tplink_kasa_on_off = "{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"on_off\": %d}}}";
const char * tplink_kasa_brightness = "{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"brightness\": %d}}}";
const char * tplink_kasa_hsv = "{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"hue\": %d, \"saturation\": %d}}}";

union payload_header
{
  char bytes[4];
  uint32_t payload_length;
};

void tplink_kasa_encrypt(const char * payload, char * encypted_payload)
{
    /* autokey cypher key value */
    char key = 171;

    /* the first 4 bytes in the encrypted data define the length of the payload, encoded in big endian */
    /* since ESP32 is little endian, need to swap the endianness */
    union payload_header header;
    header.payload_length = strlen(payload);
    encypted_payload[0] = header.bytes[3];
    encypted_payload[1] = header.bytes[2];
    encypted_payload[2] = header.bytes[1];
    encypted_payload[3] = header.bytes[0];

    /* XOR each byte with the previous encypted byte or 171 for the first byte */
    for (int i = 0; i < header.payload_length; i++)
    {
        key = encypted_payload[i + sizeof(header)] = payload[i] ^ key;
    }
}
