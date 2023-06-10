import paho.mqtt.subscribe as subscribe
import paho.mqtt.publish as publish
import pyconfig

def on_message(client, userdata, message):
    print(message.topic)
    print(message.payload)
    # Check if the topic and payload match the desired condition
#    if message.topic == 'v3/flamex@ttn/devices/eui-70b3d57ed005d56f/up' and message.payload == b'geofence':
    # Publish a new message
    if (message.topic == "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/join"):
        publish.single(
            "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/down/replace",
            payload='{"downlinks":[{"f_port": 15,"frm_payload":"MzUzMDJlMzAzMDMwMzAzMDMwMzAzMDMwMzAzMDMwMmMzMTMyMmUzMDMwMzAzMDMwMzAzMDMwMzAzMDMwMzAyYzM1MzAyZTMwMzAzMDMwMzAzMDMwMzAzMDMwMzAzMDJjMzEzMjJlMzAzMDMwMzAzMDMwMzAzMDMwMzAzMDMw","priority": "HIGH"}]}',
            hostname="eu1.cloud.thethings.network",
            port=1883,
            auth={'username': pyconfig.USERNAME, 'password': pyconfig.PASSWORD}
        )
        print('ho pubblicato il primo messaggio')

    

subscribe.callback(
    on_message,
    topics=['#'],
    hostname="eu1.cloud.thethings.network",
    port=1883,
    auth={'username': pyconfig.USERNAME, 'password': pyconfig.PASSWORD}
)
