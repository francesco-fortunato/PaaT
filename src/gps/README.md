# Paat using Wi-Fi connection and MQTT

#### UART PIN CONFIGURATION ON ESP32 HELTEC LORA32 WIFI (V2)

In order to let the E108-GN02D GPS module work with ESP32 Heltec Lora V2 in Riot OS, we should edit the default pins of UART configuration:

Open the file `RIOT/boards/esp32-heltec-lora32-v2/include/periph_conf.h`

Edit the lines:

```
#ifndef UART1_TXD
#define UART1_TXD   GPIO10  /**< direct I/O pin for UART_DEV(1) TxD */
#endif
#ifndef UART1_RXD
#define UART1_RXD   GPIO9   /**< direct I/O pin for UART_DEV(1) RxD */
```

To make them like this:

```
#ifndef UART1_TXD
#define UART1_TXD   GPIO17  /**< direct I/O pin for UART_DEV(1) TxD */
#endif
#ifndef UART1_RXD
#define UART1_RXD   GPIO16   /**< direct I/O pin for UART_DEV(1) RxD */
```

Then connect the TXD of the GPS to PIN 16 and the RXD of the GPS to PIN 17
