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
#include "crypto/aes.h"
#include "led.h"
#define DELAY               (60 * US_PER_SEC)


// MQTT client settings
#define BUF_SIZE 1024

#define MQTT_BROKER_ADDR "192.168.155.13" // After connecting to the hotspot with the pc, insert here the ifconfig wlo1
#define MQTT_TOPIC "sensor/gps"
#define MQTT_VERSION_v311 4 /* MQTT v3.1.1 version is 4 */
#define COMMAND_TIMEOUT_MS 4000

#define DEFAULT_MQTT_CLIENT_ID ""

#define DEFAULT_MQTT_USER ""

#define DEFAULT_MQTT_PWD ""

#define DEFAULT_MQTT_PORT 1883 // Default MQTT port

#define DEFAULT_KEEPALIVE_SEC 10 // Keepalive timeout in seconds

#define IS_RETAINED_MSG 0

#define PRINT_KEY_LINE_LENGTH 5

#define BUZZER_PIN          GPIO23

static MQTTClient client;
static Network network;

static unsigned char buf[BUF_SIZE];
static unsigned char readbuf[BUF_SIZE];

#define GPS_UART_DEV UART_DEV(1)
#define GPS_BAUDRATE 9600
#define GPS_CE_PIN GPIO2

float latitude = 0;
float longitude = 0;
int satellitesNum = 0;
bool soundOn = false;
bool lightOn = false;
float geofenceMinLat = 0;
float geofenceMaxLat = 0;
float geofenceMinLng = 0;
float geofenceMaxLng = 0;
bool geofenceViolated = false;
int nextLoRaWanWakeUp = 0;

cipher_context_t cyctx;
uint8_t key[AES_KEY_SIZE_128] = "PeShVmYq3s6v9yfB";
uint8_t cipher[AES_KEY_SIZE_128];

char nmea_buffer[MINMEA_MAX_SENTENCE_LENGTH];
struct minmea_sentence_gga frame;
int i = 0;
char c;

float geofence[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
char* sub_topic_geofence = "geofence";
char* emergence_topic = "actuators/";
char *topic = MQTT_TOPIC;

char stack1[THREAD_STACKSIZE_MAIN];
char stack2[THREAD_STACKSIZE_MAIN];
kernel_pid_t thread_pid_buzzer;
kernel_pid_t thread_pid_led;

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

static int mqtt_disconnect(void)
{

    int res = MQTTDisconnect(&client);
    if (res < 0) {
        printf("mqtt_example: Unable to disconnect\n");
    }
    else {
        printf("mqtt_example: Disconnect successful\n");
    }

    NetworkDisconnect(&network);
    return res;
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

static void _on_msg_received(MessageData *data)
{
    printf("gps_mqtt: message received on topic"
           " %.*s: %.*s\n",
           (int)data->topicName->lenstring.len,
           data->topicName->lenstring.data, (int)data->message->payloadlen,
           (char *)data->message->payload);

    // Extract the received string
    char *message = (char *)data->message->payload;

    // If the topic is geofence, update geofence
    if (strcmp(data->topicName->lenstring.data, "geofence") == 0){

        int count = 0;

        // Parse the string and extract the geofence
        char *token = strtok(message, "[,]");
        while (token != NULL && count < 8)
        {
            geofence[count] = atof(token);
            token = strtok(NULL, ",");
            count++;
        }

        // Geofence will have this form: {lat1, lon1, lat2, lon2, ...}

        // Assign value to maxLat and minLat
        for (int i = 0; i < 8; i += 2) {
            float latitude = geofence[i];
            if (i == 0) {
                geofenceMinLat = latitude;
                geofenceMaxLat = latitude;
            } else {
                if (latitude < geofenceMinLat) {
                geofenceMinLat = latitude;
                }
                if (latitude > geofenceMaxLat) {
                geofenceMaxLat = latitude;
                }
            }
        }

        // Assign value to maxLon and minLon
        for (int i = 1; i < 8; i += 2) {
            float longitude = geofence[i];
            if (i == 1) {
                geofenceMinLng = longitude;
                geofenceMaxLng = longitude;
            } else {
                if (longitude < geofenceMinLng) {
                geofenceMinLng = longitude;
                }
                if (longitude > geofenceMaxLng) {
                geofenceMaxLng = longitude;
                }
            }
        }

        printf("\nMaxLat: %f, MinLng: %f, MinLat: %f, MaxLng: %f\n",
                geofenceMaxLat, geofenceMinLng, geofenceMinLat, geofenceMaxLng);

        // Print the extracted geofence
        for (int i = 0; i < count; i++)
        {
            printf("Value %d: %.12f\n", i + 1, geofence[i]);
        }
    }
    else if (strcmp(data->topicName->lenstring.data, "actuators/") == 0) {

        int count = 0;
        char *token = strtok(message, "[,]");

        // String is "['true', 'true']"
        
        while (token != NULL && count < 2) {
            if (count == 0) {
                lightOn = (strcmp(token, "'true'") == 0 || strcmp(token, "true") == 0) ? true : false;
                token = strtok(NULL, "', ");
                count++;
            } else {
                soundOn = (strcmp(token, "'true'") == 0 || strcmp(token, "true") == 0) ? true : false;
                count++;
            }
        }
        
        printf("Light: %s, Sound: %s\n", lightOn ? "true" : "false", soundOn ? "true" : "false");
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
    printf("Unencrypted: %s\n", msg);

    size_t msg_len = strlen(msg);
    size_t padding_len = 16 - (msg_len % 16);
    size_t padded_len = msg_len + padding_len;
    uint8_t padded_msg[padded_len];

    memcpy(padded_msg, msg, msg_len);
    memset(padded_msg + msg_len, 0, padding_len);

    aes_init(&cyctx, key, AES_KEY_SIZE_128);

    print_bytes(padded_msg, padded_len);

    uint8_t* encrypted_msg = malloc(padded_len);
    if (encrypted_msg == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    for (int i = 0; i < (int)padded_len; i += 16)
    {
        aes_encrypt(&cyctx, padded_msg + i, encrypted_msg + i);
    }

    printf("Encrypted:\n");
    print_bytes(encrypted_msg, padded_len);

    // Convert encrypted_msg to hex string
    char* hex_string = malloc((padded_len * 2) + 1);
    if (hex_string == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }

    bytes_to_hex_string(encrypted_msg, padded_len, hex_string);

    return (uint8_t*) hex_string;
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
                //create new temp as int latit and longit. then parse in float and assign them to
                //latitude and longitude
                
                //int latit = frame.latitude.value;
                //int longit = frame.longitude.value;
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

// Led Thread
void *toggle_led_thread(void *arg){

    (void) arg;
    while(1){    
        if (lightOn){
            LED_ON(0);
        }
        else{
            printf("LIGHT OFF\n");
            LED_OFF(0);
            thread_sleep();
        }
    }
    return NULL;
}

// Buzzer Thead
void *buzzer_thread(void *arg)
{
    (void) arg;
    while(1) {
        if(soundOn) {
            printf("BUZZ ON\n");
            gpio_set(BUZZER_PIN);
            xtimer_usleep(400000);  // Wait for 400ms
            gpio_clear(BUZZER_PIN);
            xtimer_usleep(1000000);  // Wait for 1s
        } else {
            printf("BUZZ OFF\n");
            thread_sleep();
        }
    }
    return NULL;
}

int main(void)
{
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    /* let LWIP initialize */
    ztimer_sleep(ZTIMER_MSEC, 10 * MS_PER_SEC);

    // Initialize network
    NetworkInit(&network);

    ztimer_sleep(ZTIMER_MSEC, 2 * MS_PER_SEC);

    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE,
                   readbuf,
                   BUF_SIZE);

    MQTTStartTask(&client);

    // Connect to MQTT broker
    mqtt_connect();

    int ask_for_geofence;

    char* message_geofence = "Geofence";

    MQTTMessage message;
    message.qos = QOS2;
    message.retained = IS_RETAINED_MSG;
    message.payload = message_geofence;
    message.payloadlen = strlen(message.payload);
    
    if (client.isconnected){
        if ((ask_for_geofence = MQTTPublish(&client, topic, &message)) < 0)
        {
            printf("mqtt_example: Unable to publish (%d)\n", ask_for_geofence);
        }
        else
        {
            printf("mqtt_example: Message (%s) has been published to topic %s "
                    "with QOS %d\n",
                    (uint8_t *)message.payload, topic, (int)message.qos);

            mqtt_disconnect();
            mqtt_connect();

            printf("Geofence: Subscribing to %s\n", sub_topic_geofence);
            int ret = MQTTSubscribe(&client,
                                    sub_topic_geofence, QOS2, _on_msg_received);
            if (ret < 0)
            {
                printf("Geofence: Unable to subscribe to %s (%d)\n",
                    sub_topic_geofence, ret);
            }
            else
            {
                printf("Geofence: Now subscribed to %s, QOS %d\n",
                    sub_topic_geofence, (int)QOS2);
            }
        }
    }

    // Wait for geofence
    while (1){
        if (geofence[0]!=0.0){
            printf("GOT GEOFENCE\n");
            int unsub = MQTTUnsubscribe(&client, sub_topic_geofence);
            if (unsub < 0) {
                printf("mqtt_example: Unable to unsubscribe from topic: %s\n", sub_topic_geofence);
                mqtt_disconnect();
            }
            else {
                printf("mqtt_example: Unsubscribed from topic:%s\n", sub_topic_geofence);
            }
            break;
        }
        else{
            printf("No geofence\n");
            xtimer_sleep(5);
        }
    }

    uart_init(GPS_UART_DEV, GPS_BAUDRATE, gps_rx_cb, NULL);
    gpio_init(GPS_CE_PIN, GPIO_OUT);

    //to be removed
    latitude = 51.896015603272;
    longitude = 12.493740198896;

    thread_pid_buzzer = thread_create(stack1, sizeof(stack1),
                    THREAD_PRIORITY_MAIN + 1,
                    THREAD_CREATE_SLEEPING,
                    buzzer_thread,
                    NULL, "buzzer_thread");
    thread_pid_led = thread_create(stack2, sizeof(stack2),
                    THREAD_PRIORITY_MAIN +1,
                    THREAD_CREATE_SLEEPING,
                    toggle_led_thread,
                    NULL, "toggle_led_thread");

    xtimer_ticks32_t last = xtimer_now();

    while (1)
    {
        if (!client.isconnected){
            mqtt_connect();
        }
        satellitesNum = 3;
        bool lorawanWakeUp = false;
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
            geofenceViolated = isInGeofence(51.00000, longitude);

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
            // encrypt msg with AES
            char* lati= "41.896015603272";
            char* longi="12.493740198896";
            uint8_t* lat_encrypted = aes_128_encrypt(lati);
            uint8_t* lon_encrypted = aes_128_encrypt(longi);

            printf("lati: %s, lat_encrypted: %s\n",
                lati, (char*)lat_encrypted);

            char json[500];
            sprintf(json, "{\"id\": \"%d\", \"Latitude\": \"%s\", \"Longitude\": \"%s\", \"Satellites\": \"%d\", \"l\": \"%s\", \"s\": \"%s\"}",
                    1, lat_encrypted, lon_encrypted, satellitesNum, lightOn ? "true" : "false", soundOn ? "true" : "false");

            uint8_t *msg_to_be_sent = (uint8_t*)json;

            // Publish gps value to MQTT broker
            MQTTMessage message;
            message.qos = QOS2;
            message.retained = IS_RETAINED_MSG;
            message.payload = msg_to_be_sent;
            message.payloadlen = strlen(message.payload);

            int rc;
            if ((rc = MQTTPublish(&client, topic, &message)) < 0)
            {
                printf("mqtt_example: Unable to publish (%d)\n", rc);
                mqtt_disconnect();
                xtimer_sleep(10);
                mqtt_connect();
            }
            else
            {
                printf("mqtt_example: Message (%s) has been published to topic %s "
                       "with QOS %d\n",
                       (uint8_t *)message.payload, topic, (int)message.qos);
            }

            free(lat_encrypted);
            free(lon_encrypted);

            mqtt_disconnect();
            mqtt_connect();

            printf("Get Light and Sound: Subscribing to %s\n", emergence_topic);
            int sub = MQTTSubscribe(&client,
                                    emergence_topic, QOS2, _on_msg_received);
            if (sub < 0)
            {
                printf("Get Light and Sound: Unable to subscribe to %s (%d)\n",
                    emergence_topic, sub);
            }
            else
            {
                printf("Get Light and Sound: Now subscribed to %s, QOS %d\n",
                    emergence_topic, (int)QOS2);
            }

            bool tmp_led = lightOn;
            bool tmp_buzz = soundOn;
            
            for(int f = 0; f<10; f++){
                if(lightOn && tmp_led==false)
                { 
                    thread_wakeup(thread_pid_led);
                }
                if(soundOn && tmp_buzz==false)
                { 
                    thread_wakeup(thread_pid_buzzer);
                }
                if (lightOn!=tmp_led || soundOn!=tmp_buzz){
                    int unsub = MQTTUnsubscribe(&client, emergence_topic);
                    if (unsub < 0) {
                        printf("mqtt_example: Unable to unsubscribe from topic: %s\n", emergence_topic);
                        mqtt_disconnect();
                    }
                    else {
                        printf("mqtt_example: Unsubscribed from topic:%s\n", emergence_topic);
                    }
                    mqtt_disconnect();
                    mqtt_connect();

                    printf("Get Light and Sound: Subscribing to %s\n", emergence_topic);
                    int sub = MQTTSubscribe(&client,
                                            emergence_topic, QOS2, _on_msg_received);
                    if (sub < 0)
                    {
                        printf("Get Light and Sound: Unable to subscribe to %s (%d)\n",
                            emergence_topic, sub);
                    }
                    else
                    {
                        printf("Get Light and Sound: Now subscribed to %s, QOS %d\n",
                            emergence_topic, (int)QOS2);
                    }
                }
                tmp_led = lightOn;
                tmp_buzz = soundOn;

                printf("inside waiting for godot loop\n"); 
                xtimer_sleep(10);
            }

                    // Wait for actuators. If no actions in 5 minutes, restart loop
            printf("i'm here\n");
        }
        else{
            printf("Geofence is not violated, who cares about position?\n");
            printf("i'm here 2\n");

            // Sleep for 10 sec
            xtimer_periodic_wakeup(&last, DELAY);
        }
    }

    return 0;
}
