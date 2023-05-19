/*
 * Copyright (C) 2023 Andrea Sepielli, Francesco Fortunato, Valerio Francione
 */

/**
 * @{
 *
 * @file
 * @brief       PAAT
 *
 * @authors      Andrea Sepielli <andreasepielli97@gmail.com>,
 *               Francesco Fortunato <francesco.fortunato1999@gmail.com>,
 *               Valerio Francione <>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include "time.h"
#include "thread.h"

#include "periph/uart.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "analog_util.h"

#include "xtimer.h"
#include "minmea.h"

#include "paho_mqtt.h"
#include "MQTTClient.h"

// MQTT client settings
#define BUF_SIZE 1024

#define MQTT_BROKER_ADDR "192.168.1.1" // After connecting to the hotspot with the pc, insert here the ifconfig wlo1
#define MQTT_TOPIC "gps"
#define MQTT_VERSION_v311 4 /* MQTT v3.1.1 version is 4 */
#define COMMAND_TIMEOUT_MS 4000

#define DEFAULT_MQTT_CLIENT_ID ""

#define DEFAULT_MQTT_USER ""

#define DEFAULT_MQTT_PWD ""

#define DEFAULT_MQTT_PORT 1883 // Default MQTT port

#define DEFAULT_KEEPALIVE_SEC 10 // Keepalive timeout in seconds

#define IS_RETAINED_MSG 0

static MQTTClient client;
static Network network;

static unsigned char buf[BUF_SIZE];
static unsigned char readbuf[BUF_SIZE];

#define GPS_UART_DEV UART_DEV(1)
#define GPS_BAUDRATE 9600

float latitude;
float longitude;
int satellitesNum;

char nmea_buffer[MINMEA_MAX_SENTENCE_LENGTH];
struct minmea_sentence_gga frame;
int i = 0;
char c;

static int mqtt_connect(void)
{
    int ret = 0;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VERSION_v311;
    data.clientID.cstring = DEFAULT_MQTT_CLIENT_ID;
    data.username.cstring = DEFAULT_MQTT_USER;
    data.password.cstring = DEFAULT_MQTT_PWD;
    data.keepAliveInterval = 60;
    data.cleansession = 1;

    printf("MQTT: Connecting to MQTT Broker from %s %d\n",
           MQTT_BROKER_ADDR, DEFAULT_MQTT_PORT);
    printf("MQTT: Trying to connect to %s, port: %d\n",
           MQTT_BROKER_ADDR, DEFAULT_MQTT_PORT);
    ret = NetworkConnect(&network, MQTT_BROKER_ADDR, DEFAULT_MQTT_PORT);
    if (ret < 0)
    {
        printf("MQTT: Unable to connect\n");
        return ret;
    }

    printf("user:%s clientId:%s password:%s\n", data.username.cstring,
           data.clientID.cstring, data.password.cstring);
    ret = MQTTConnect(&client, &data);
    if (ret < 0)
    {
        printf("MQTT: Unable to connect client %d\n", ret);
        int res = MQTTDisconnect(&client);
        if (res < 0)
        {
            printf("MQTT: Unable to disconnect\n");
        }
        else
        {
            printf("MQTT: Disconnect successful\n");
        }

        NetworkDisconnect(&network);
        return res;
    }
    else
    {
        printf("MQTT: Connection successfully\n");
    }

    printf("MQTT client connected to broker\n");
    return 0;
}

static void gps_rx_cb(void *arg, uint8_t data)
{
    (void)arg;
    // putchar(data);
    if (data != '\n')
    {
        nmea_buffer[i++] = data;
    }
    else
    {
        nmea_buffer[i] = '\0';

        switch (minmea_sentence_id(nmea_buffer, false))
        {
        case MINMEA_SENTENCE_GGA:
        {
            if (minmea_parse_gga(&frame, nmea_buffer))
            {
                printf("$GGA: fix quality: %d\n", frame.fix_quality);
                // long degreesLat = frame.latitude.value / 1000000;
                // long minutesLat = frame.latitude.value % 1000000;
                // double lat = degreesLat + (double)minutesLat / 600000;
                // // int degreesLon = frame.longitude.value / 1000000;
                // // int minutesLon = frame.longitude.value % 1000000;
                // // float lon = degreesLon + (float)minutesLon / 600000;

                // printf("LATITUDE: %f\n", lat);
                // printf("LONGITUDE: %f\n", lon);
                printf("LATITUDE: %d\n", frame.latitude.value);
                printf("LONGITUDE: %d\n", frame.longitude.value);
                // printf("ALTITUDE: %f %c\n", (float)frame.altitude.value/100, frame.altitude_units);
                printf("SATELLITES: %d\n", frame.satellites_tracked);

                // SENT MESSAGE

                char json[200];
                sprintf(json, "{\"id\": \"%d\", \"Latitude\": \"%d\", \"Longitude\": \"%d\", \"Altitude\": \"%d %c\", \"Satellites\": \"%d\"}",
                        1, frame.latitude.value, frame.longitude.value, frame.altitude.value, frame.altitude_units, frame.satellites_tracked);

                char *msg = json;
                // MQTT
                //  Publish flame value to MQTT broker
                MQTTMessage message;
                message.qos = QOS2;
                message.retained = IS_RETAINED_MSG;
                message.payload = msg;
                message.payloadlen = strlen(message.payload);

                char *topic = "gps";

                int rc;
                if ((rc = MQTTPublish(&client, topic, &message)) < 0)
                {
                    printf("mqtt_example: Unable to publish (%d)\n", rc);
                }
                else
                {
                    printf("mqtt_example: Message (%s) has been published to topic %s "
                           "with QOS %d\n",
                           (char *)message.payload, topic, (int)message.qos);
                }
            }
        }
        break;
        default:
            break;
        }

        i = 0;
    }
}

int main(void)
{
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    /* let LWIP initialize */
    ztimer_sleep(ZTIMER_MSEC, 5 * MS_PER_SEC);

    // Initialize network
    NetworkInit(&network);

    ztimer_sleep(ZTIMER_MSEC, 2 * MS_PER_SEC);

    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE,
                   readbuf,
                   BUF_SIZE);

    MQTTStartTask(&client);

    // Connect to MQTT broker
    mqtt_connect();

    uart_init(GPS_UART_DEV, GPS_BAUDRATE, gps_rx_cb, NULL);
    while (1)
    {
        xtimer_sleep(5);
    }

    return 0;
}
