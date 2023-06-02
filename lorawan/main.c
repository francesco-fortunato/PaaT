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

/* By default, messages are sent every 20s to respect the duty cycle
   on each channel */
#ifndef SEND_PERIOD_S
#define SEND_PERIOD_S       (20U)
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

const char* coordinates[] = {
    "{\"lat\": 41.8960156032722, \"lng\": 12.493740198896651}",
    "{\"lat\": 41.89167421134482, \"lng\": 12.498581879314175}",
    "{\"lat\": 41.88977635297827, \"lng\": 12.49825531512865}",
    "{\"lat\": 41.89339070237489, \"lng\": 12.495170117908552}",
    "{\"lat\": 41.89059371609803, \"lng\": 12.496922330983208}",
    "{\"lat\": 41.891863645396185, \"lng\": 12.496194385513437}",
    "{\"lat\": 41.89459139551559, \"lng\": 12.49689715015853}",
    "{\"lat\": 41.89867101472699, \"lng\": 12.499329545754307}",
    "{\"lat\": 41.895111125939, \"lng\": 12.49598538346839}",
    "{\"lat\": 41.89599950800407, \"lng\": 12.493933689160949}"
};

int size = sizeof(coordinates) / sizeof(coordinates[0]);


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


static void _send_message(void) {
    int size = sizeof(coordinates) / sizeof(coordinates[0]);

    // Generate random index
    int random_index = rand() % size;

    // Get the random coordinate string
    const char* random_coordinate = coordinates[random_index];

    // Allocate memory for the message string dynamically
    size_t message_length = strlen(random_coordinate) + 1;
    char* message = (char*)malloc(message_length);
    if (message == NULL) {
        printf("Failed to allocate memory for message\n");
        return;
    }

    // Copy the random coordinate string to the message
    strncpy(message, random_coordinate, message_length);

    printf("Sending: %s\n", message);

    /* Try to send the message */
    // Send as JSON format
    uint8_t ret = semtech_loramac_send(&loramac, (uint8_t*)message, message_length - 1);
    if (ret != SEMTECH_LORAMAC_TX_DONE) {
        printf("Cannot send message '%s', ret code: %d\n", message, ret);
        free(message);  // Release the allocated memory
        return;
    }

    free(message);  // Release the allocated memory
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
    puts("LoRaWAN Class A low-power application");
    puts("=====================================");

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
        puts("Starting join procedure");
        if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
            puts("Join procedure failed");
            return 1;
        }

#ifdef MODULE_PERIPH_EEPROM
        /* Save current MAC state to EEPROM */
        semtech_loramac_save_config(&loramac);
#endif
    }
#endif

#ifdef USE_ABP /* ABP activation mode */
    /* Convert identifiers and keys strings to byte arrays */
    fmt_hex_bytes(devaddr, CONFIG_LORAMAC_DEV_ADDR_DEFAULT);
    fmt_hex_bytes(nwkskey, CONFIG_LORAMAC_NWK_SKEY_DEFAULT);
    fmt_hex_bytes(appskey, CONFIG_LORAMAC_APP_SKEY_DEFAULT);
    semtech_loramac_set_devaddr(&loramac, devaddr);
    semtech_loramac_set_nwkskey(&loramac, nwkskey);
    semtech_loramac_set_appskey(&loramac, appskey);

    /* Configure RX2 parameters */
    semtech_loramac_set_rx2_freq(&loramac, CONFIG_LORAMAC_DEFAULT_RX2_FREQ);
    semtech_loramac_set_rx2_dr(&loramac, CONFIG_LORAMAC_DEFAULT_RX2_DR);

#ifdef MODULE_PERIPH_EEPROM
    /* Store ABP parameters to EEPROM */
    semtech_loramac_save_config(&loramac);
#endif

    /* Use a fast datarate, e.g. BW125/SF7 in EU868 */
    semtech_loramac_set_dr(&loramac, LORAMAC_DR_5);

    /* ABP join procedure always succeeds */
    semtech_loramac_join(&loramac, LORAMAC_JOIN_ABP);
#endif
    puts("Join procedure succeeded");

    /* start the sender thread */
    sender_pid = thread_create(sender_stack, sizeof(sender_stack),
                               SENDER_PRIO, 0, sender, NULL, "sender");

    /* trigger the first send */
    msg_t msg;
    msg_send(&msg, sender_pid);
    return 0;
}
