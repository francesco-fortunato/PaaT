# Evaluation Report

This document serves as the final evaluation report for the project. For the previous version, please refer to [this link](https://github.com/francesco-fortunato/PaaT/blob/main/docs/Evaluation.md).

To evaluate the product we studied and investigated the connection via LoRa and the power consumption and management,
due to the activation of GPS only in case the animal is inside the geofence, and GPS combined with buzzer and LED in case the animal is outside the geofence instead.
The evaluation primarily focused on two key aspects: the LoRaWAN connection and power consumption and management. Detailed investigations were conducted to understand and to assess the power requirements and consumption patterns of the prototype. The following sections provide a thorough overview of the evaluation findings.

## LoRaWAN

To evaluate the LoRaWAN connection, a specialized tool offered by FIT IoT-LAB was utilized. The developed code for transmitting data was uploaded to the tool, and a series of tests were conducted. The test results demonstrated the performance of the LoRaWAN connection, with specific attention given to power consumption during data transmission.

The obtained results are graphically represented in the figure below:

![LoRa test](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/LoRa%20test.png)

The graph reveals distinctive peaks in power consumption corresponding to message transmissions. Notably, the highest peak corresponds to the sending of messages, while the smallest peak aligns with the reception of response messages. To further analyze these peaks, detailed breakdowns are presented in the subsequent figures:

- Sending and response messages in detail:

![Sending and response messages in details](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Send-Receive.png)

- Power consumption during message transmission:
![Send message](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Send.png)

- Power consumption during response message reception:
![Response message](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Receive.png)

## Power Consumption Assessment

A comprehensive assessment of power consumption was conducted to gain a comprehensive understanding of the product's energy needs. The evaluation took into account the power requirements of the entire system. Thanks to the tool provided by FIT IoT-LAB, it was possible to accurately determine the power consumption specific to the radio component. This was achieved by calculating the area under the curve of power consumption during uplink transmission.

Uplink: (Base x Height) + (Base x Height) = (0.2 x 0.19) + (0.04 x 0.033) = 0.03932 W 

Similarly, the power consumption for receiving a response message was calculated as:

Downlink: Base x Height = 0.1 x 0.055 = 0.0055 W 

## System Autonomy Considerations

To provide a complete view of the system's power requirements, a comprehensive power consumption table has been prepared, which includes all relevant sensor components:

![Power Consumption Table](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Power%20consumption%20table.png)

By analyzing the collected consumption data, it becomes possible to make accurate considerations regarding the system's autonomy. To this end, an estimation table was created, outlining the system's consumption based on various sampling times:

![Estimation Table](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Estimation%20table.png)

The proposed approach suggests utilizing a sampling time of 5 minutes when the pet ventures beyond the geofence. With a 2400 mAh battery, the system is estimated to maintain a minimum autonomy of 1 day under these circumstances. Conversely, if the pet remains within the geofence, a sampling time of 1 hour is recommended. Based on the earlier power consumption figures and employing the same 2400 mAh battery, the system can operate for approximately 2 days.

## Future improvements

From the autonomy consuìiderations, we see that the major consumption derives from the board, that because in Riot, at the moment, is not possible to use the deep sleep mode of the board which guarantee a greater energy savings during system downtime.

We have also studied this possibility and recalculated the values inherent in power consumption if we had the option of using this mode, and from that we have provided the new table:

![New Power Consumption Table](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Power%20Consumption%20Table%20in%20deep%20sleep.png)

Under the new hypothesis we have redefined the consumption of the system. In the first row it is possible to notice the huge battery saving when deep sleep mode is introduced whenever the pet is located inside the geofence. In the second row are shown the new values of consumption with smaller power cycles of light and sound (0.4 sec ON / 1 sec OFF): 

![New Estimation Table](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Estimation%20Table%20deep%20sleep.png)

The new values of consumption open a new scenario in which with a battery of 2400mAh and a sampling time of 1 hour, if the device stays always inside the geofence the battery can last up to 35.5 days.
Outside geofence can be also noticed a reduction in consumption of about 500mAh.

