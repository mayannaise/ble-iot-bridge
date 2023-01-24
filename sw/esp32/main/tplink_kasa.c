/**
 * @file Constants and functions for communicating with TP-Link Kasa IoT smart devices
 */

/* system includes */
#include <string.h>
#include <esp_log.h>

/* local includes */
#include "tplink_kasa.h"
#include "wifi.h"

union payload_header
{
    char bytes[4];
    uint32_t payload_length;
};

const char cipher_key = 171;

void tplink_kasa_decrypt(const char * encrypted_payload, char * decrypted_payload)
{
    /* autokey cypher key value */
    char key = cipher_key;

    /* the first 4 bytes in the encrypted data define the length of the payload, encoded in big endian */
    /* since ESP32 is little endian, need to swap the endianness */
    union payload_header header;
    header.bytes[3] = encrypted_payload[0];
    header.bytes[2] = encrypted_payload[1];
    header.bytes[1] = encrypted_payload[2];
    header.bytes[0] = encrypted_payload[3];

    /* XOR each byte with the previous encypted byte or 171 for the first byte */
    for (int i = 0; i < header.payload_length; i++)
    {
        decrypted_payload[i] = encrypted_payload[i + sizeof(header)] ^ key;
        key = encrypted_payload[i + sizeof(header)];
    }

    /* stick a null on the end to terminate the string */
    decrypted_payload[header.payload_length] = '\0';
}

void tplink_kasa_encrypt(const char * payload, char * encrypted_payload)
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

    /* XOR each byte with the previous encypted byte or 171 for the first byte */
    for (int i = 0; i < header.payload_length; i++)
    {
        key = encrypted_payload[i + sizeof(header)] = payload[i] ^ key;
    }
}
