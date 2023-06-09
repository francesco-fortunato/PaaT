/*
 * Copyright (C) 2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example demonstrating the use of LoRaWAN with RIOT
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
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
#define DELAY (60 * US_PER_SEC)

#define PRINT_KEY_LINE_LENGTH 5


/* By default, messages are sent every 300s to respect the duty cycle
   on each channel */
#ifndef SEND_PERIOD_S
#define SEND_PERIOD_S       (60U)
#endif

/* Low-power mode level */
#define PM_LOCK_LEVEL       (1)

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
#define GPS_CE_PIN GPIO_PIN(2, 0)

static msg_t _recv_queue[RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];

// Encryption 
cipher_context_t cyctx;
uint8_t key[AES_KEY_SIZE_128] = "PeShVmYq3s6v9yfB";
uint8_t cipher[AES_KEY_SIZE_128];

uint8_t *msg_to_be_sent;

float geofence[8] = {41.89869814775031, 12.499697811345177, 41.89869814775031, 12.488901615142824, 41.894850104580236, 12.488901615142824, 41.894850104580236, 12.499697811345177};
/*
const char* coordinates[] = {
    "{\"lat\": 41.8960156032722, \"lng\": 12.493740198896651}",
    "{\"lat\": 41.89167421134482, \"lng\": 12.498581879314175}",
    "{\"lat\": 41.88977635297827, \"lng\": 12.49825531512865}",
    "{\"lat\": 41.89339070237489, \"lng\": 12.495170117908552}",
    "{\"lat\": 41.89059371609803, \"lng\": 12.496922330983208}",
    "{\"lat\": 41.891863645396185, \"lng\": 12.496194385513437}",
    "{\"lat\": 41.89459139551559, \"lng\": 12.49689715015853}",
    "{\"lat\": 41.89867101472699, \"lng\": 12.499329545754307}"
};*/
float latitude = 0.0;
float longitude = 0.0;
int satellitesNum = 0;
bool soundOn = false;
bool lightOn = false;
float geofenceMinLat = 0;
float geofenceMaxLat = 0;
float geofenceMinLng = 0;
float geofenceMaxLng = 0;
bool geofenceViolated;

static bool isInGeofence(float latitude, float longitude)
{
    // TBD: THRESHOLD ERROR
    if (latitude > geofenceMaxLat || latitude < geofenceMinLat || longitude > geofenceMaxLng || longitude < geofenceMinLng)
    {
        printf("GEOFENCE IS VIOLATED\n");
        return true;
    }
    else
    {
        return false;
    }
}

// Function to print bytes 
void print_bytes(const uint8_t *key, size_t size) // to be removed
{
    for (size_t i = 0; i < size; i++)
    {
        if (i != 0 && i % PRINT_KEY_LINE_LENGTH == 0)
            printf("\n");
        printf("%02x", key[i]);
    }
    printf("\n");
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

    //print_bytes(padded_msg, padded_len);

    uint8_t* encrypted_msg = malloc(padded_len);
    if (encrypted_msg == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    for (int i = 0; i < (int)padded_len; i += 16)
    {
        aes_encrypt(&cyctx, padded_msg + i, encrypted_msg + i);
    }

    //printf("Encrypted:\n");
    //print_bytes(encrypted_msg, padded_len);

    // Convert encrypted_msg to hex string
    char* hex_string = malloc((padded_len * 2) + 1);
    if (hex_string == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    bytes_to_hex_string(encrypted_msg, padded_len, hex_string);

    return (uint8_t*) hex_string;
}


/*static void _send_message(uint8_t* lat_encrypted, uint8_t* lon_encrypted)
{
    // Try to send the message 

    // Calculate the required message length
    size_t message_length = 32 + 32 + 20; // "{\"lat\":\"\",\"lon\":\"\"}"

    // Allocate memory for the message string dynamically
    char *message = (char *)malloc(message_length);
    if (message == NULL)
    {
        printf("Failed to allocate memory for message\n");
        return;
    }

    // Construct the JSON message
    snprintf(message, message_length, "{\"lat\":\"%s\",\"lon\":\"%s\"}",
             lat_encrypted, lon_encrypted);

    printf("Sending: %s\n", message);
    uint8_t ret = semtech_loramac_send(&loramac, (uint8_t*)message, strlen(message)-1);
    if (ret != SEMTECH_LORAMAC_TX_DONE)
    {
        printf("Cannot send message '%s', ret code: %d\n", message, ret);
        free(message);
        return;
    }

    free(message);
    pm_set(PM_LOCK_LEVEL);
}*/

static void *_recv(void *arg)
{
    msg_init_queue(_recv_queue, RECV_MSG_QUEUE);
    (void)arg;
    while (1) {
        /* blocks until a message is received */
        semtech_loramac_recv(&loramac);
        loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
        printf("Data received: %s, port: %d\n",
               (char *)loramac.rx_data.payload, loramac.rx_data.port);
            thread_sleep();
    }
    return NULL;
}

int main(void)
{
    // Seed the random number generator
    printf("LoRaWAN Class A low-power application\n");
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
        printf("Starting join procedure\n");
        if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
            printf("Join procedure failed\n");
            return 1;
        }

#ifdef MODULE_PERIPH_EEPROM
        /* Save current MAC state to EEPROM */
        semtech_loramac_save_config(&loramac);
#endif
    }
#endif
    printf("Join procedure succeeded\n");

    /* start the sender thread */
                            
    
    // Assign value to maxLat and minLat
    for (int i = 0; i < 8; i += 2)
    {
        float latGF = geofence[i];
        if (i == 0)
        {
            geofenceMinLat = latGF;
            geofenceMaxLat = latGF;
        }
        else
        {
            if (latGF < geofenceMinLat)
            {
                geofenceMinLat = latGF;
            }
            if (latGF > geofenceMaxLat)
            {
                geofenceMaxLat = latGF;
            }
        }
    }

    // Assign value to maxLon and minLon
    for (int i = 1; i < 8; i += 2)
    {
        float lonGF = geofence[i];
        if (i == 1)
        {
            geofenceMinLng = lonGF;
            geofenceMaxLng = lonGF;
        }
        else
        {
            if (lonGF < geofenceMinLng)
            {
                geofenceMinLng = lonGF;
            }
            if (lonGF > geofenceMaxLng)
            {
                geofenceMaxLng = lonGF;
            }
        }
    }

    printf("\nMaxLat: %f, MinLng: %f, MinLat: %f, MaxLng: %f\n",
            geofenceMaxLat, geofenceMinLng, geofenceMinLat, geofenceMaxLng);

    recv_pid = thread_create(_recv_stack, sizeof(_recv_stack),
            THREAD_PRIORITY_MAIN + 1, 0, _recv, NULL, "recv thread");

    xtimer_ticks32_t last = xtimer_now();

    while (1) {

        // Wait for reliable position or timeout
        satellitesNum=4;

        latitude = 51.8960156;
        longitude = 12.4937401988;

        //if (satellitesNum >= 4)
        {
            geofenceViolated = isInGeofence(latitude, longitude);
        }

        if (geofenceViolated)
        {
            char *lati = "51.896015620074";
            char *longi ="12.242154256234";
            // encrypt msg with AES
            printf("lati: %s, longi:%s\n", lati, longi);
            uint8_t* lat_encrypted = (uint8_t *)aes_128_encrypt(lati);

            printf("lat: %s\n", lat_encrypted);

            uint8_t* lon_encrypted = (uint8_t *)aes_128_encrypt(longi);

            printf("longi: %s\n", lon_encrypted);

            char json[500];
            sprintf(json, "{\"lat\": \"%s\", \"lng\": \"%s\"}",
                    lat_encrypted, lon_encrypted);

            msg_to_be_sent = (uint8_t *)json;

            printf("Sending: %s\n", msg_to_be_sent);
            uint8_t ret = semtech_loramac_send(&loramac, (uint8_t*)msg_to_be_sent, strlen(json));
            if (ret != SEMTECH_LORAMAC_TX_DONE)
            {
                printf("Cannot send message '%s', ret code: %d\n", msg_to_be_sent, ret);
                free(msg_to_be_sent);
            }
            

            // Wait for actuators. If no actions in 5 minutes, restart loop
            printf("i'm here\n");

            free(lat_encrypted);
            free(lon_encrypted);

            xtimer_sleep(30);

            //thread_wakeup(recv_pid);

            printf("after thread\n");
            
            xtimer_sleep(30);
        }
        else
        {
            printf("Geofence is not violated, who cares about position?\n");
            printf("i'm here 2\n");

            // Sleep for 1h
            xtimer_periodic_wakeup(&last, DELAY);
        }
    }
}
