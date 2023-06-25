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
