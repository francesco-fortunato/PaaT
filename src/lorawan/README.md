# PaaT based on LoRa

This folder contains the main.c file of the app using LoRa, along with the Makefile.

This code is used to make simulation on FIT IOT-LAB by using the board st-lrwan1 in the Saclay site. Compile it using `make all term`, then inside bin/b-l072z-lrwan1 you will find the lorawan.elf file. 

Let's go through the code and understand its main components:

#### Joining the LoRaWAN Network:

The application is using Over-The-Air Activation (OTAA), so it converts identifiers and keys to byte arrays.
The code sets the LoRaWAN data rate and checks if the device has already joined the network. If not, it attempts to join the network using the joinLoRaNetwork() function, a function defined since sometimes the join is not successful, which the purpose is to retry if the join fails.
If the network join is successful, the configuration is optionally saved to EEPROM.

#### Sending Geofence Request:

The code allocates memory for a geofence request message and constructs the JSON message using the "geofence" keyword.
The message is then sent using the semtech_loramac_send() function.

#### Receiving Geofence Data:

After a delay of 2 seconds, the code wakes up a receiver thread (_recv) to receive geofence data.
The received message is parsed and the geofence values are extracted and stored in the geofence array.

#### Geofence Violation Checking:

The code enters an infinite loop where it waits for a reliable position or a timeout.
In this example, latitude and longitude values are hardcoded for demonstration purposes.

The code checks if the number of satellites is greater than or equal to 4, and if so, determines if the current position is within the geofence using the isInGeofence() function. The code is simulating a scenario in which when the board is initialized, the pet is inside the geofence, while after 10 minutes it exit.

If the geofence is violated, is turned on an LED, is encrypted and sent the latitude and longitude coordinates using AES encryption, and is performed a sleep for 300 seconds.

If the geofence is not violated, it prints a message, turns off the LED, sets soundOn and lightOn flags to false, and sleeps for 300 seconds. Of course this because there is no need to know the actual position of the pet since it is inside the geofence.