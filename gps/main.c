#include <stdio.h>
#include <string.h>
#include "periph/uart.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include "minmea.h"

#define GPS_UART_DEV UART_DEV(1)
#define GPS_BAUDRATE 9600

float latitude;
float longitude;
float altitude;
int satellitesNum;

char nmea_buffer[MINMEA_MAX_SENTENCE_LENGTH];
struct minmea_sentence_gga frame;
int i = 0;
char c;

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
                printf("ALTITUDE: %d %c\n", frame.altitude.value, frame.altitude_units);
                // printf("ALTITUDE: %f %c\n", (float)frame.altitude.value/100, frame.altitude_units);
                printf("SATELLITES: %d\n", frame.satellites_tracked);
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
    uart_init(GPS_UART_DEV, GPS_BAUDRATE, gps_rx_cb, NULL);
    while (1)
    {
        xtimer_sleep(5);
    }

    return 0;
}
