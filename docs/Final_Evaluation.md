# EVALUATION

This is the final evaluation file. The previous version could be found [here](https://github.com/francesco-fortunato/PaaT/blob/main/docs/Evaluation.md).

To evaluate the product we studied and investigated the connection via LoRa and the power consumption and management,
due to the activation of GPS only in case the animal is inside the geofence, and GPS combined with buzzer and LED in case the animal is outside the geofence instead.

# LoRawan connection

As for the lora test, we used the tool offered by Fit-Iot lab. Here we uploaded the code we developed for sending the data, and these are the results obtained:

![LoRa test](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/LoRa%20test.png)

As we can see, there are picks of consumption in correspondence with the sending of messages, the highest one correspond to the sending message, 
insead the smallest correspond to the response message. Below is possible to see that picks more in detail:

![Sending and response messages in details](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Send-Receive.png)

![Send message](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Send.png)

![Response message](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Receive.png)

# Power consumption

Thanks to the test provided with Fit-Iot lab, we could determinate the consumption for the radio part, simply calculating the area delimited by the curve of the messages. So we have:

Send message: (Base x Height) + (Base x Height) = (0.2 x 0.19) + (0.04 x 0.033) = 0.03932 W 

Response message: Base x Height = 0.1 x 0.055 = 0.0055 W 

To calculate the consumption of the entire system we have obviously add the consumption of all the sensors, to do that we had provided this table:
![Power consumption table](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Power%20consumption%20table.png)

Given that data about the consumption we can do some consideration inherent the autonomy of the system, to do that we have provided the consumption of the whole system based on various sampling time: 

![Estimation table](https://github.com/francesco-fortunato/PaaT/blob/main/docs/img/Estimation%20table.png)

Our idea is to use a sampling time of 5 minutes when the pet go out the geofence, and in this case, with a battery of 2400 mAh, we could say that the autonomy of the system is at least of 1 day.
In case the pet remain inside the geofence we provide a sampling time of 1 hour, and given the consumption above, with the same battery of 2400 mAh the system could work for 2 days.
