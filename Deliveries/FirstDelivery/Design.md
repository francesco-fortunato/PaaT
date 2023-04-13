# Design

## Conceptual Architecture
The pet tracking system consists of two main hardware components, an ESP32 v2 Heltec LoRa and GPS module, a battery, a buzzer,. The ESP32 is responsible for collecting location data from the GPS module, processing the data, and sending it to a cloud service for storage and analysis. The buzzer and LED are used to alert the pet owner when their pet leaves the designated area.

The cloud service is responsible for storing the pet's location data, analyzing the data, and sending alerts to the pet owner when their pet leaves the designated area. The pet owner can access the cloud service through a mobile app or web interface.

## Hardware Components
ESP32 v2 Heltec LoRa and GPS Module
The ESP32 v2 Heltec LoRa and GPS module is the primary hardware component of the pet tracking system. It is responsible for collecting location data from the GPS module, processing the data, and sending it to the cloud service for storage and analysis. The module has a built-in LoRa transceiver that allows it to communicate with the cloud service over long distances.

Battery
The battery is used to power the ESP32 v2 Heltec LoRa and GPS module. The battery life will depend on how often the system is used.

Buzzer and LED
The buzzer and LED are used to alert the pet owner when their pet leaves the designated area. The buzzer emits a loud sound, while the LED flashes.

GPS Module
The GPS module is responsible for collecting location data. The module communicates with the ESP32 v2 Heltec LoRa module over a serial connection.

## Software Components
Core Algorithms
The pet tracking system uses several core algorithms to function correctly. The algorithms are responsible for collecting location data, processing the data, and sending alerts to the pet owner when their pet leaves the designated area.

Network Protocols
The pet tracking system uses the LoRaWAN protocol to communicate between the ESP32 v2 Heltec LoRa and GPS module and the cloud service. LoRaWAN is a low-power, wide-area network protocol that allows for long-range communication between devices.

APIs Used
The pet tracking system uses several APIs to interact with the cloud service. The APIs are responsible for storing location data, analyzing the data, and sending alerts to the pet owner.

Network Architecture
The pet tracking system consists of IoT elements, Edge components, Cloud components, and End-user components. The IoT elements consist of the ESP32 v2 Heltec LoRa and GPS module, battery, buzzer, and LED. The Edge components consist of the LoRaWAN gateway, which receives data from the IoT elements and sends it to the cloud service. The Cloud components consist of the cloud service, which stores location data, analyzes the data, and sends alerts to the pet owner. The End-user components consist of the mobile app or web interface, which the pet owner uses to access the cloud service. The network architecture is depicted in the following diagram:

                
                +-------------+
                |      DB     |
                +-------------+
                         |
                         |
                         |
             +----------------+
             | Cloud Service  |
             +----------------+
                         |
                         |
                         |
           +-----------------+
           | LoRaWAN Gateway |
           +-----------------+
                         |
                         |
                         |
          +-------------------+
          | ESP32 v2 Heltec  |
          | LoRa and GPS     |
          | Module, Battery, |
          | Buzzer, LED      |
          +-------------------+
