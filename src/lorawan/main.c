/*
 * Copyright (C) 2023 Francesco Fortunato, Valerio Francione, Andrea Sepielli
 *
 * This file is subject to the terms and conditions of the MIT License.
 * See the file LICENSE in the top level directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example demonstrating the use of LoRaWAN with RIOT
 *
 * @authors      Francesco Fortunato <francesco.fortunato1999@gmail.com>,
 *               Valerio Francione <francione97@gmail.com>,
 *               Andrea Sepielli <andreasepielli97@gmail.com>
 *
 * @}
 */


#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include <time.h>
#include "random.h"

#include "periph/uart.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "analog_util.h"

#include "msg.h"
#include "thread.h"
#include "fmt.h"

#include "periph/pm.h"
#if IS_USED(MODULE_PERIPH_RTC)
#include "periph/rtc.h"
#else
#include "timex.h"
#include "ztimer.h"
#endif
#include "xtimer.h"

#include "net/loramac.h"
#include "semtech_loramac.h"
#include "crypto/aes.h"

#include "led.h"

#define PRINT_KEY_LINE_LENGTH 5


/* By default, messages are sent every 300s (5min) to respect the duty cycle
   on each channel */
#ifndef SEND_PERIOD_S
#define SEND_PERIOD_S       (60U)
#endif

/* Low-power mode level */
#define PM_LOCK_LEVEL       (1)

#define MAX_JOIN_RETRIES 3

char* geofence[4];

static kernel_pid_t recv_pid;
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

extern semtech_loramac_t loramac;
#if !IS_USED(MODULE_PERIPH_RTC)
static ztimer_t timer;
#endif

#ifdef USE_OTAA
static uint8_t deveui[LORAMAC_DEVEUI_LEN];
static uint8_t appeui[LORAMAC_APPEUI_LEN];
static uint8_t appkey[LORAMAC_APPKEY_LEN];
#endif

#ifdef USE_ABP
static uint8_t devaddr[LORAMAC_DEVADDR_LEN];
static uint8_t nwkskey[LORAMAC_NWKSKEY_LEN];
static uint8_t appskey[LORAMAC_APPSKEY_LEN];
#endif

#define RECV_MSG_QUEUE                   (4U)

static msg_t _recv_queue[RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

// Encryption 
cipher_context_t cyctx;
uint8_t key[AES_KEY_SIZE_128] = "PeShVmYq3s6v9yfB";
uint8_t cipher[AES_KEY_SIZE_128];

uint8_t *msg_to_be_sent;
char* msg_received = NULL;

char* latitude;
char* longitude;
int satellitesNum = 0;
bool soundOn = false;
bool lightOn = false;
float geofenceMinLat = 0;
float geofenceMaxLat = 0;
float geofenceMinLng = 0;
float geofenceMaxLng = 0;
bool geofenceViolated;

static bool isInGeofence(char* latitude, char* longitude)
{
    // TBD: THRESHOLD ERROR
    //The strcmp function returns a value greater than 0 if the first string is lexicographically greater than the second string, 
    //a value less than 0 if the first string is lexicographically smaller than the second string, 
    //and 0 if both strings are equal.
    int maxlatComparison = strcmp(latitude, geofence[0]);
    int maxlngComparison = strcmp(longitude, geofence[1]);
    int minlatComparison = strcmp(latitude, geofence[2]);
    int minlngComparison = strcmp(longitude, geofence[3]);
    /*
    if (maxlatComparison > 0)
    {
        printf("lat > geofence[0] (maxlat): %d\n", maxlatComparison);
    }
    else if (maxlatComparison < 0)
    {
        printf("lat < geofence[0] (maxlat): %d\n", maxlatComparison);
    }
    else
    {
        printf("lat = geofence[0] (maxlat): %d\n", maxlatComparison);
    }

    if (maxlngComparison > 0)
    {
        printf("lng > geofence[1] (maxlng): %d\n", maxlngComparison);
    }
    else if (maxlngComparison < 0)
    {
        printf("lng < geofence[1] (maxlng): %d\n", maxlngComparison);
    }
    else
    {
        printf("lng = geofence[1] (maxlng): %d\n", maxlngComparison);
    }

    if (minlatComparison > 0)
    {
        printf("lat > geofence[2] (minlat): %d\n", minlatComparison);
    }
    else if (minlatComparison < 0)
    {
        printf("lat < geofence[2] (minlat): %d\n", minlatComparison);
    }
    else
    {
        printf("lat = geofence[2] (minlat): %d\n", minlatComparison);
    }

    if (minlngComparison > 0)
    {
        printf("lng > geofence[3] (minlng): %d\n", minlngComparison);
    }
    else if (minlngComparison < 0)
    {
        printf("lng < geofence[3] (minlng): %d\n", minlngComparison);
    }
    else
    {
        printf("lng = geofence[3] (minlng): %d\n", minlngComparison);
    }
    */
    // Compare latitude and longitude strings with geofence boundaries using strcmp
    if (maxlatComparison > 0 || maxlngComparison > 0 || minlatComparison < 0 || minlngComparison < 0)
    {
        printf("GEOFENCE IS VIOLATED\n");
        return true;
    }
    else
    {
        return false;
    }
}

// Function to convert bytes to hex string
void bytes_to_hex_string(const uint8_t *bytes, size_t size, char *hex_string)
{
    size_t index = 0;

    for (size_t i = 0; i < size; i++)
    {
        // Format the byte as a two-digit hexadecimal string
        sprintf(&hex_string[index], "%02x", bytes[i] & 0xff);

        // Move the index two positions forward
        index += 2;
    }
    hex_string[index]='\0';
}

// Encrypt msg using AES-128
uint8_t* aes_128_encrypt(const char* msg)
{
    //printf("Unencrypted: %s\n", msg);

    size_t msg_len = strlen(msg);
    size_t padding_len = 16 - (msg_len % 16);
    size_t padded_len = msg_len + padding_len;
    uint8_t padded_msg[padded_len];

    memcpy(padded_msg, msg, msg_len);
    memset(padded_msg + msg_len, 0, padding_len);

    aes_init(&cyctx, key, AES_KEY_SIZE_128);

    uint8_t* encrypted_msg = malloc(padded_len);
    if (encrypted_msg == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    for (int i = 0; i < (int)padded_len; i += 16)
    {
        aes_encrypt(&cyctx, padded_msg + i, encrypted_msg + i);
    }

    // Convert encrypted_msg to hex string
    char* hex_string = malloc((padded_len * 2) + 1);
    if (hex_string == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    bytes_to_hex_string(encrypted_msg, padded_len, hex_string);

    return (uint8_t*) hex_string;
}

static void* _recv(void* arg) {
    msg_init_queue(_recv_queue, RECV_MSG_QUEUE);
    (void)arg;
    /* blocks until a message is received */
    semtech_loramac_recv(&loramac);
    loramac.rx_data.payload[loramac.rx_data.payload_len] = '\0';
    printf("PAYLOAD IS %s\n", (char*)loramac.rx_data.payload);

    
    // Allocate memory for msg_received
    msg_received = (char*)malloc(loramac.rx_data.payload_len + 1);
    if (msg_received == NULL) {
        // Handle memory allocation error
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Copy payload into msg_received
    strncpy(msg_received, (char*)loramac.rx_data.payload, loramac.rx_data.payload_len);
    msg_received[loramac.rx_data.payload_len] = '\0';
    thread_zombify();
    return NULL;
}

bool joinLoRaNetwork(void) {

    int joinRetries = 0;

    while (joinRetries < MAX_JOIN_RETRIES) {
        /* Start the Over-The-Air Activation (OTAA) procedure to retrieve the
         * generated device address and to get the network and application session
         * keys.
         */
        printf("Starting join procedure (attempt %d)\n", joinRetries + 1);
        if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) == SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
            printf("Join procedure succeeded\n");
            return true; // Join successful, return true
        } else {
            printf("Join procedure failed\n");
            joinRetries++;
        }
    }

    printf("Exceeded maximum join retries\n");
    return false; // Join failed after maximum retries, return false
}
/*
 * Copyright (C) 2023 Francesco Fortunato, Valerio Francione, Andrea Sepielli
 *
 * This file is subject to the terms and conditions of the MIT License.
 * See the file LICENSE in the top level directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example demonstrating the use of LoRaWAN with RIOT
 *
 * @authors      Francesco Fortunato <francesco.fortunato1999@gmail.com>,
 *               Valerio Francione <francione97@gmail.com>,
 *               Andrea Sepielli <andreasepielli97@gmail.com>
 *
 * @}
 */

#include "board.h"
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include <time.h>
#include "random.h"

#include "periph/uart.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "analog_util.h"

#include "msg.h"
#include "thread.h"
#include "fmt.h"

#include "periph/pm.h"
#if IS_USED(MODULE_PERIPH_RTC)
#include "periph/rtc.h"
#else
#include "timex.h"
#include "ztimer.h"
#endif
#include "xtimer.h"

#include "net/loramac.h"
#include "semtech_loramac.h"
#include "crypto/aes.h"

#include "led.h"

#define PRINT_KEY_LINE_LENGTH 5


/* By default, messages are sent every 300s (5min) to respect the duty cycle
   on each channel */
#ifndef SEND_PERIOD_S
#define SEND_PERIOD_S       (60U)
#endif

/* Low-power mode level */
#define PM_LOCK_LEVEL       (1)

#define MAX_JOIN_RETRIES 3

char* geofence[4];

static kernel_pid_t recv_pid;
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

extern semtech_loramac_t loramac;
#if !IS_USED(MODULE_PERIPH_RTC)
static ztimer_t timer;
#endif

#ifdef USE_OTAA
static uint8_t deveui[LORAMAC_DEVEUI_LEN];
static uint8_t appeui[LORAMAC_APPEUI_LEN];
static uint8_t appkey[LORAMAC_APPKEY_LEN];
#endif

#ifdef USE_ABP
static uint8_t devaddr[LORAMAC_DEVADDR_LEN];
static uint8_t nwkskey[LORAMAC_NWKSKEY_LEN];
static uint8_t appskey[LORAMAC_APPSKEY_LEN];
#endif

#define RECV_MSG_QUEUE                   (4U)

#define TIME_TO_CHANGE 138347 // 6 minuti

static msg_t _recv_queue[RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

// Encryption 
cipher_context_t cyctx;
uint8_t key[AES_KEY_SIZE_128] = "PeShVmYq3s6v9yfB";
uint8_t cipher[AES_KEY_SIZE_128];

uint8_t *msg_to_be_sent;
char* msg_received = NULL;

char* latitude;
char* longitude;
int satellitesNum = 0;
bool soundOn = false;
bool lightOn = false;
float geofenceMinLat = 0;
float geofenceMaxLat = 0;
float geofenceMinLng = 0;
float geofenceMaxLng = 0;
bool geofenceViolated;
bool geofence_tmp = false;

static bool isInGeofence(char* latitude, char* longitude)
{
    // TBD: THRESHOLD ERROR
    //The strcmp function returns a value greater than 0 if the first string is lexicographically greater than the second string, 
    //a value less than 0 if the first string is lexicographically smaller than the second string, 
    //and 0 if both strings are equal.
    int maxlatComparison = strcmp(latitude, geofence[0]);
    int maxlngComparison = strcmp(longitude, geofence[1]);
    int minlatComparison = strcmp(latitude, geofence[2]);
    int minlngComparison = strcmp(longitude, geofence[3]);
    /*
    if (maxlatComparison > 0)
    {
        printf("lat > geofence[0] (maxlat): %d\n", maxlatComparison);
    }
    else if (maxlatComparison < 0)
    {
        printf("lat < geofence[0] (maxlat): %d\n", maxlatComparison);
    }
    else
    {
        printf("lat = geofence[0] (maxlat): %d\n", maxlatComparison);
    }

    if (maxlngComparison > 0)
    {
        printf("lng > geofence[1] (maxlng): %d\n", maxlngComparison);
    }
    else if (maxlngComparison < 0)
    {
        printf("lng < geofence[1] (maxlng): %d\n", maxlngComparison);
    }
    else
    {
        printf("lng = geofence[1] (maxlng): %d\n", maxlngComparison);
    }

    if (minlatComparison > 0)
    {
        printf("lat > geofence[2] (minlat): %d\n", minlatComparison);
    }
    else if (minlatComparison < 0)
    {
        printf("lat < geofence[2] (minlat): %d\n", minlatComparison);
    }
    else
    {
        printf("lat = geofence[2] (minlat): %d\n", minlatComparison);
    }

    if (minlngComparison > 0)
    {
        printf("lng > geofence[3] (minlng): %d\n", minlngComparison);
    }
    else if (minlngComparison < 0)
    {
        printf("lng < geofence[3] (minlng): %d\n", minlngComparison);
    }
    else
    {
        printf("lng = geofence[3] (minlng): %d\n", minlngComparison);
    }
    */
    // Compare latitude and longitude strings with geofence boundaries using strcmp
    if (maxlatComparison > 0 || maxlngComparison > 0 || minlatComparison < 0 || minlngComparison < 0)
    {
        printf("GEOFENCE IS VIOLATED\n");
        return true;
    }
    else
    {
        return false;
    }
}

// Function to convert bytes to hex string
void bytes_to_hex_string(const uint8_t *bytes, size_t size, char *hex_string)
{
    size_t index = 0;

    for (size_t i = 0; i < size; i++)
    {
        // Format the byte as a two-digit hexadecimal string
        sprintf(&hex_string[index], "%02x", bytes[i] & 0xff);

        // Move the index two positions forward
        index += 2;
    }
    hex_string[index]='\0';
}

// Encrypt msg using AES-128
uint8_t* aes_128_encrypt(const char* msg)
{
    //printf("Unencrypted: %s\n", msg);

    size_t msg_len = strlen(msg);
    size_t padding_len = 16 - (msg_len % 16);
    size_t padded_len = msg_len + padding_len;
    uint8_t padded_msg[padded_len];

    memcpy(padded_msg, msg, msg_len);
    memset(padded_msg + msg_len, 0, padding_len);

    aes_init(&cyctx, key, AES_KEY_SIZE_128);

    uint8_t* encrypted_msg = malloc(padded_len);
    if (encrypted_msg == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    for (int i = 0; i < (int)padded_len; i += 16)
    {
        aes_encrypt(&cyctx, padded_msg + i, encrypted_msg + i);
    }

    // Convert encrypted_msg to hex string
    char* hex_string = malloc((padded_len * 2) + 1);
    if (hex_string == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    bytes_to_hex_string(encrypted_msg, padded_len, hex_string);

    return (uint8_t*) hex_string;
}

static void* _recv(void* arg) {
    msg_init_queue(_recv_queue, RECV_MSG_QUEUE);
    (void)arg;
    /* blocks until a message is received */
    semtech_loramac_recv(&loramac);
    loramac.rx_data.payload[loramac.rx_data.payload_len] = '\0';
    printf("PAYLOAD IS %s\n", (char*)loramac.rx_data.payload);

    
    // Allocate memory for msg_received
    msg_received = (char*)malloc(loramac.rx_data.payload_len + 1);
    if (msg_received == NULL) {
        // Handle memory allocation error
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Copy payload into msg_received
    strncpy(msg_received, (char*)loramac.rx_data.payload, loramac.rx_data.payload_len);
    msg_received[loramac.rx_data.payload_len] = '\0';
    thread_zombify();
    return NULL;
}

bool joinLoRaNetwork(void) {

    int joinRetries = 0;

    while (joinRetries < MAX_JOIN_RETRIES) {
        /* Start the Over-The-Air Activation (OTAA) procedure to retrieve the
         * generated device address and to get the network and application session
         * keys.
         */
        printf("Starting join procedure (attempt %d)\n", joinRetries + 1);
        if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) == SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
            printf("Join procedure succeeded\n");
            return true; // Join successful, return true
        } else {
            printf("Join procedure failed\n");
            joinRetries++;
        }
    }

    printf("Exceeded maximum join retries\n");
    return false; // Join failed after maximum retries, return false
}

int main(void)
{
    // Seed the random number generator
    printf("PaaT: a LoRaWAN Class A low-power application\n");
    printf("=====================================\n");

    /*
     * Enable deep sleep power mode (e.g. STOP mode on STM32) which
     * in general provides RAM retention after wake-up.
     */
#if IS_USED(MODULE_PM_LAYERED)
    for (unsigned i = 1; i < PM_NUM_MODES - 1; ++i) {
        pm_unblock(i);
    }
#endif

#ifdef USE_OTAA /* OTAA activation mode */
    /* Convert identifiers and keys strings to byte arrays */
    fmt_hex_bytes(deveui, CONFIG_LORAMAC_DEV_EUI_DEFAULT);
    fmt_hex_bytes(appeui, CONFIG_LORAMAC_APP_EUI_DEFAULT);
    fmt_hex_bytes(appkey, CONFIG_LORAMAC_APP_KEY_DEFAULT);
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    /* Use a fast datarate, e.g. BW125/SF7 in EU868 */
    semtech_loramac_set_dr(&loramac, LORAMAC_DR_5);

    /* Join the network if not already joined */
    if (!semtech_loramac_is_mac_joined(&loramac)) {
        /* Start the Over-The-Air Activation (OTAA) procedure to retrieve the
         * generated device address and to get the network and application session
         * keys.
         */    
        if (!joinLoRaNetwork()) {
            printf("Failed to join the network\n");
            return 1;
        }

#ifdef MODULE_PERIPH_EEPROM
        /* Save current MAC state to EEPROM */
        semtech_loramac_save_config(&loramac);
#endif
    }
#endif

    /* start the sender thread */
    recv_pid = thread_create(_recv_stack, sizeof(_recv_stack),
            THREAD_PRIORITY_MAIN - 1, 0, _recv, NULL, "recv thread");

    xtimer_sleep(5);
    
    char* geo_request = (char*)malloc(strlen("geofence")+1);
    if (geo_request == NULL)
    {
        printf("Failed to allocate memory for message\n");
        return 1;
    }

    // Construct the JSON message
    snprintf(geo_request, strlen("geofence")+1, "geofence");

    printf("Sending: %s\n", geo_request);
    uint8_t req = semtech_loramac_send(&loramac, (uint8_t*)geo_request, strlen(geo_request));
    if (req != SEMTECH_LORAMAC_TX_DONE)
    {
        printf("Cannot send message '%s', ret code: %d\n", geo_request, req);
    }

    // Initialize the LED pin as an output
    gpio_init(LED1_PIN_NUM, GPIO_OUT);

    // Turn on LED1 by setting the GPIO pin to high
    gpio_set(LED1_PIN_NUM);

    free(geo_request);

    xtimer_sleep(2);

    thread_wakeup(recv_pid);    

    int count = 0;

    // Parse the string and extract the geofence
    char* token = strtok(msg_received, ",");
    while (token != NULL && count < 4)
    {
        geofence[count] = (char*)malloc(strlen(token) + 1); // Allocate memory for the geofence value
        strcpy(geofence[count], token); // Copy the geofence value
        printf("geofence[%d] = %s\n", count, geofence[count]);
        token = strtok(NULL, ",");
        count++;
    }

    free(msg_received);

    thread_kill_zombie(recv_pid);

    gpio_clear(LED1_PIN_NUM);


    char* latitude;
    char* longitude;

    count = 1;

    xtimer_sleep(20);

    while (1) {
        if (count >=3 && count < 6){
            latitude = "51.896800742599";
            longitude = "12.489154256234";
        }
        else{
            latitude = "41.896800742599";
            longitude = "12.489154256234";
        }

        // Wait for reliable position or timeout
        satellitesNum=4;

        printf("Latitude : %s, Longitude : %s\n", latitude, longitude);
        //if (satellitesNum >= 4)
        {
            geofenceViolated = isInGeofence(latitude, longitude);
        }

        if (geofenceViolated)
        {
            geofence_tmp = true;

            lightOn = true;
            soundOn = true;

            gpio_set(LED1_PIN_NUM);

            
            // encrypt msg with AES
            uint8_t* lat_encrypted = (uint8_t *)aes_128_encrypt(latitude);

            printf("Encrypted lat: %s\n", lat_encrypted);

            uint8_t* lon_encrypted = (uint8_t *)aes_128_encrypt(longitude);

            printf("Encrypted lon: %s\n", lon_encrypted);

            char json[500];
            sprintf(json, "{\"lat\": \"%s\", \"lng\": \"%s\", \"s\": \"%d\"}",
                    lat_encrypted, lon_encrypted, satellitesNum);

            msg_to_be_sent = (uint8_t *)json;

            printf("Sending: %s\n", msg_to_be_sent);
            uint8_t ret = semtech_loramac_send(&loramac, (uint8_t*)msg_to_be_sent, strlen(json));
            if (ret != SEMTECH_LORAMAC_TX_DONE)
            {
                printf("Cannot send message '%s', ret code: %d\n", msg_to_be_sent, ret);
                free(msg_to_be_sent);
            }
            
            free(lat_encrypted);
            free(lon_encrypted);
            
            xtimer_sleep(300);
                    
        }
        else
        {
            if (lightOn) gpio_clear(LED1_PIN_NUM);
            soundOn = false;
            lightOn = false;

            if (geofence_tmp){
                
                // encrypt msg with AES
                uint8_t* lat_encrypted = (uint8_t *)aes_128_encrypt(latitude);

                printf("Encrypted lat: %s\n", lat_encrypted);

                uint8_t* lon_encrypted = (uint8_t *)aes_128_encrypt(longitude);

                printf("Encrypted lon: %s\n", lon_encrypted);

                char json[500];
                sprintf(json, "{\"lat\": \"%s\", \"lng\": \"%s\", \"s\": \"%d\"}",
                        lat_encrypted, lon_encrypted, satellitesNum);

                msg_to_be_sent = (uint8_t *)json;

                printf("Sending: %s\n", msg_to_be_sent);
                uint8_t ret = semtech_loramac_send(&loramac, (uint8_t*)msg_to_be_sent, strlen(json));
                if (ret != SEMTECH_LORAMAC_TX_DONE)
                {
                    printf("Cannot send message '%s', ret code: %d\n", msg_to_be_sent, ret);
                    free(msg_to_be_sent);
                }
                
                free(lat_encrypted);
                free(lon_encrypted);
                geofence_tmp = false;
            }

            printf("Geofence not violated\n");
            
            xtimer_sleep(300);
        }
        count++;
    }
}
