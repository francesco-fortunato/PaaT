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
 *               Valerio Francione <francione97@gmail.com>
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
#define MQTT_TOPIC "sensor/gps"
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
#define GPS_CE_PIN GPIO2

int latitude = 0;
int longitude = 0;
int satellitesNum = 0;
bool soundOn = false;
bool lightOn = false;
int geofenceMinLat = 0;
int geofenceMaxLat = 0;
int geofenceMinLng = 0;
int geofenceMaxLng = 0;
bool geofenceViolated = false;
int nextLoRaWanWakeUp = 0;

char nmea_buffer[MINMEA_MAX_SENTENCE_LENGTH];
struct minmea_sentence_gga frame;
int i = 0;
char c;

float values[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

static void _on_msg_received(MessageData *data)
{
    printf("paho_mqtt_example: message received on topic"
           " %.*s: %.*s\n",
           (int)data->topicName->lenstring.len,
           data->topicName->lenstring.data, (int)data->message->payloadlen,
           (char *)data->message->payload);

    // Extract the received string
    char* message = (char*)data->message->payload;

    int count = 0;

    // Parse the string and extract the values
    char* token = strtok(message, "[,]");
    while (token != NULL && count < 8) {
        values[count] = atof(token);
        token = strtok(NULL, ",");
        count++;
    }

    // Print the extracted values
    for (int i = 0; i < count; i++) {
        printf("Value %d: %.12f\n", i + 1, values[i]);
    }
}

static int mqtt_unsub(void)
{
    int unsub = MQTTUnsubscribe(&client, sub_topic);

    if (unsub < 0) {
        printf("mqtt_example: Unable to unsubscribe from topic: %s\n", sub_topic);
    }
    else {
        printf("mqtt_example: Unsubscribed from topic:%s\n", sub_topic);
    }
    return unsub;
}

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
                latitude = frame.latitude.value;
                longitude = frame.longitude.value;
                satellitesNum = frame.satellites_tracked;
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

    char* sub_topic = "geofence";


    printf("Geofence: Subscribing to %s\n", sub_topic);
    int ret = MQTTSubscribe(&client,
              sub_topic, QOS2, _on_msg_received);
    if (ret < 0) {
        printf("Geofence: Unable to subscribe to %s (%d)\n",
               sub_topic, ret);
    }
    else {
        printf("Geofence: Now subscribed to %s, QOS %d\n",
               sub_topic, (int) QOS2);
    }

    uart_init(GPS_UART_DEV, GPS_BAUDRATE, gps_rx_cb, NULL);
    gpio_init(GPS_CE_PIN, GPIO_OUT);
    while (1)
    {
        latitude = 0;
        longitude = 0;
        satellitesNum = 0;
        bool lorawanWakeUp = false;
        if (nextLoRaWanWakeUp <= xtimer_now())
        {
            lorawanWakeUp = true;
        }
        // GPS power on
        gpio_write(GPS_CE_PIN, 1);
        printf("GPS power on\n");

        // Wait for reliable position or timeout
        int checks = 0;
        while (checks < 10 && satellitesNum < 3)
        {
            xtimer_sleep(10); // TBD
            checks++;
        }

        // GPS power off
        gpio_write(GPS_CE_PIN, 0);
        printf("GPS power off\n");

        if (satellitesNum >= 3)
        {
            // check if the geofence is violated (if exists)
            if (geofenceMaxLat != 0 && geofenceMaxLng != 0 && geofenceMinLat != 0 && geofenceMinLng != 0)
            {
                if (latitude > geofenceMaxLat || latitude < geofenceMinLat || longitude > geofenceMaxLng || longitude < geofenceMinLng)
                {
                    geofenceViolated = true;
                }
                else
                {
                    geofenceViolated = false;
                }
            }

            // if geofence violated set sleep for LoRaWAN to 5min else 1h
            if (geofenceViolated)
            {
                nextLoRaWanWakeUp = xtimer_now() + 300000;
            }
            else
            {
                nextLoRaWanWakeUp = xtimer_now() + 3600000;
            }
        }

        if (geofenceViolated || lorawanWakeUp)
        {
            // subscribe to check if new geofence published
            // parse response message and set lightOn and soundOn, edit geofence if updated
            // lightOn = false;
            // soundOn = false;
            // geofenceMinLat = 0;
            // geofenceMaxLat = 0;
            // geofenceMinLng = 0;
            // geofenceMaxLng = 0;
            // compute if the new geofence is violated if updated
            bool geofenceUpdated = false; // to be removed
            if (geofenceUpdated)
            {
                // check if the geofence is violated (if exists)
                if (geofenceMaxLat != 0 && geofenceMaxLng != 0 && geofenceMinLat != 0 && geofenceMinLng != 0)
                {
                    if (latitude > geofenceMaxLat || latitude < geofenceMinLat || longitude > geofenceMaxLng || longitude < geofenceMinLng)
                    {
                        geofenceViolated = true;
                    }
                    else
                    {
                        geofenceViolated = false;
                    }
                }

                // if geofence violated set sleep for LoRaWAN to 5min else 1h
                if (geofenceViolated)
                {
                    nextLoRaWanWakeUp = xtimer_now() + 300000;
                    // send new MQTT message for violation???
                }
                else
                {
                    nextLoRaWanWakeUp = xtimer_now() + 3600000;
                }
            }

            // send MQTT message
            char json[200];
            sprintf(json, "{\"id\": \"%d\", \"Latitude\": \"%d\", \"Longitude\": \"%d\", \"Satellites\": \"%d\", \"GeofenceViolated\": %s}",
                    1, latitude, longitude, satellitesNum, geofenceViolated ? "true" : "false");

            char *msg = json;
            // MQTT
            //  Publish flame value to MQTT broker
            MQTTMessage message;
            message.qos = QOS2;
            message.retained = IS_RETAINED_MSG;
            message.payload = msg;
            message.payloadlen = strlen(message.payload);

            char *topic = MQTT_TOPIC;

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

            // wait for response message or timeout
            xtimer_sleep(10); // TODO
        }

        // Sleep for 5 minutes
        xtimer_sleep(300);
    }

    return 0;
}
