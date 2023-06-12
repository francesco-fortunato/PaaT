import paho.mqtt.subscribe as subscribe
import paho.mqtt.publish as publish
import pyconfig
import base64
import random
import json
import time

counter = 0

def on_message(client, userdata, message):
    global counter  # Use the global counter variable

    print(message.topic)
    print(message.payload)
    payload_str = message.payload.decode('utf-8')  # Convert bytes to string
    payload_json = json.loads(payload_str)
    array = ["false,false", "true,false", "false,true", "true,true"]

    # Check if the topic and payload match the desired condition
#    if message.topic == 'v3/flamex@ttn/devices/eui-70b3d57ed005d56f/up' and message.payload == b'geofence':
    # Publish a new message
    if (message.topic == "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/join"):
        publish.single(
            "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/down/replace",
            payload='{"downlinks":[{"f_port": 2,"frm_payload":"NTAuMDAwMDAwMDAwMDAwLDEyLjAwMDAwMDAwMDAwMCw1MC4wMDAwMDAwMDAwMDAsMTIuMDAwMDAwMDAwMDAw","priority": "HIGH"}]}',
            hostname="eu1.cloud.thethings.network",
            port=1883,
            auth={'username': pyconfig.USERNAME, 'password': pyconfig.PASSWORD}
        )
        print('ho pubblicato il primo messaggio')
    else:
        if message.topic == "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/up" and message.topic != "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/down/queued":
            frm_payload = payload_json['uplink_message']['frm_payload']
            print(frm_payload)
            if frm_payload != "Z2VvZmVuY2U=":

                random_index = random.randint(0, 3)
                random_value = base64.b64encode(array[random_index].encode()).decode()
                print(array[random_index])
                string = "push"
                if counter>5:
                    string = "replace"
                    counter = 0
                publish.single(
                    "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/down/" + string,
                    payload='{"downlinks":[{"f_port": 2,"frm_payload":"' + random_value + '","confirmed": true, "priority": "HIGH"}]}',
                    hostname="eu1.cloud.thethings.network",
                    port=1883,
                    auth={'username': pyconfig.USERNAME, 'password': pyconfig.PASSWORD}
                )
                print('Published '+random_value)

    counter += 1
    print("counter = "+str(counter))
    

subscribe.callback(
    on_message,
    topics=['#'],
    hostname="eu1.cloud.thethings.network",
    port=1883,
    auth={'username': pyconfig.USERNAME, 'password': pyconfig.PASSWORD}
)
