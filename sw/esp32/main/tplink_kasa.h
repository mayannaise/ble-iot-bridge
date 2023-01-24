/**
 * @file Constants and functions for communicating with TP-Link Kasa IoT smart devices
 */

#ifndef INTELLILIGHT_TPLINK_KASA_H
#define INTELLILIGHT_TPLINK_KASA_H


/* system includes */
#include <string.h>
#include <unistd.h>

/* local includes */
#include "wifi.h"


/**
 * @brief Decrypt using XOR Autokey Cipher with starting key of 171
 * @param encrypted_payload Input payload to decrypt
 * @param decrypted_payload Output decrypted payload
 */
void tplink_kasa_decrypt(const char * encrypted_payload, char * decrypted_payload);

/**
 * @brief Encrypt using XOR Autokey Cipher with starting key of 171
 * @param payload Input payload to encrypt
 * @param encrypted_payload Output encrypted payload
 */
void tplink_kasa_encrypt(const char * payload, char * encypted_payload);

#endif
