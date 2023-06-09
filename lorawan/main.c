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

#include "net/loramac.h"
#include "semtech_loramac.h"
#include "crypto/aes.h"

#define PRINT_KEY_LINE_LENGTH 5


/* By default, messages are sent every 30s to respect the duty cycle
   on each channel */
#ifndef SEND_PERIOD_S
#define SEND_PERIOD_S       (30U)
#endif

/* Low-power mode level */
#define PM_LOCK_LEVEL       (1)

#define SENDER_PRIO         (THREAD_PRIORITY_MAIN - 1)
static kernel_pid_t sender_pid;
static char sender_stack[THREAD_STACKSIZE_MAIN / 2];

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

float geofence[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

const char* coordinates[] = {
    "{\"lat\": 41.8960156032722, \"lng\": 12.493740198896651}",
    "{\"lat\": 41.89167421134482, \"lng\": 12.498581879314175}",
    "{\"lat\": 41.88977635297827, \"lng\": 12.49825531512865}",
    "{\"lat\": 41.89339070237489, \"lng\": 12.495170117908552}",
    "{\"lat\": 41.89059371609803, \"lng\": 12.496922330983208}",
    "{\"lat\": 41.891863645396185, \"lng\": 12.496194385513437}",
    "{\"lat\": 41.89459139551559, \"lng\": 12.49689715015853}",
    "{\"lat\": 41.89867101472699, \"lng\": 12.499329545754307}"
};

int size = sizeof(coordinates) / sizeof(coordinates[0]);
int random_index;

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

static void _alarm_cb(void *arg)
{
    (void)arg;
    msg_t msg;
    msg_send(&msg, sender_pid);
}

static void _prepare_next_alarm(void)
{
#if IS_USED(MODULE_PERIPH_RTC)
    struct tm time;
    rtc_get_time(&time);
    // Set initial alarm to trigger every 10 minutes
    time.tm_sec += SEND_PERIOD_S;
    mktime(&time);
    rtc_set_alarm(&time, _alarm_cb, NULL);
#else
    timer.callback = _alarm_cb;
    ztimer_set(ZTIMER_MSEC, &timer, SEND_PERIOD_S * MS_PER_SEC); // Set timer to trigger every 10 minutes
#endif
}


static void _send_message(void)
{
    char *lati = "41.24533";
    char *longi = "12.242154";
    uint8_t *lat_encrypted = (uint8_t *)aes_128_encrypt(lati);
    uint8_t *lon_encrypted = (uint8_t *)aes_128_encrypt(longi);

    // Calculate the required message length
    size_t message_length = 32 + 32 + 20; // "{\"lat\":\"\",\"lon\":\"\"}"

    // Allocate memory for the message string dynamically
    char *message = (char *)malloc(message_length);
    if (message == NULL)
    {
        printf("Failed to allocate memory for message\n");
        free(lat_encrypted);
        free(lon_encrypted);
        return;
    }

    // Construct the JSON message
    snprintf(message, message_length, "{\"lat\":\"%s\",\"lon\":\"%s\"}",
             lat_encrypted, lon_encrypted);

    printf("Sending: %s\n", message);

    /* Try to send the message */
    // Send as JSON format
    uint8_t ret = semtech_loramac_send(&loramac, (uint8_t *)message, message_length - 1);
    if (ret != SEMTECH_LORAMAC_TX_DONE)
    {
        printf("Cannot send message '%s', ret code: %d\n", message, ret);
        free(message);
        free(lat_encrypted);
        free(lon_encrypted);
        return;
    }

    free(message);
    free(lat_encrypted);
    free(lon_encrypted);
    pm_set(PM_LOCK_LEVEL);
}

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
    }
    return NULL;
}

static void *sender(void *arg)
{
    (void)arg;

    msg_t msg;
    msg_t msg_queue[8];
    msg_init_queue(msg_queue, 8);

    while (1) {
        msg_receive(&msg);

        /* Trigger the message send */
        _send_message();

        /* Enter sleep mode to conserve power */
        pm_set(PM_LOCK_LEVEL);

        /* Schedule the next wake-up alarm */
        _prepare_next_alarm();
    }

    /* this should never be reached */
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
                            
    thread_create(_recv_stack, sizeof(_recv_stack),
            THREAD_PRIORITY_MAIN - 1, 0, _recv, NULL, "recv thread");
    sender_pid = thread_create(sender_stack, sizeof(sender_stack),
                                SENDER_PRIO, THREAD_CREATE_STACKTEST, sender, NULL, "sender");
    /* trigger the first send */
    msg_t msg;
    msg_send(&msg, sender_pid);
    return 0;
}
