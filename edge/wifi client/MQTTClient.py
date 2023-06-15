from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
from decimal import Decimal
import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
import signal
import pyconfig
import boto3
import base64
import re


# Set the callback function for MQTT messages
def on_message(_client, _userdata, message):
    """Handle messages"""

    print('-----')

    # Take incoming message
    payload = message.payload

    print(payload)

    if (payload == b'Geofence'):
        dynamodb = boto3.resource('dynamodb',
                              aws_access_key_id=AWS_ACCESS_KEY_ID,
                              aws_secret_access_key=AWS_SECRET_ACCESS_KEY,
                              region_name='us-east-1')  # Replace with your desired region

        # Retrieve the DynamoDB table
        table = dynamodb.Table('geofence_data')

        # Query the table and get the last item
        response = table.scan()
        items = response['Items']
        sorted_items = sorted(items, key=lambda x: x['sample_time'], reverse=True)  # sample_time is my timestamp attribute name
        last_item = sorted_items[0]['device_data'][0]

        print('Last item:', last_item)

        # Combine all elements into a single dictionary
        combined_item = []
        for i, item in enumerate(last_item, 1):
            #combined_item.append(f'lat{i}')
            combined_item.append(float(item['lat']))
            #combined_item.append(f'lng{i}')
            combined_item.append(float(item['lng']))


        # Convert the combined item to a JSON string
        json_payload = json.dumps(combined_item)

        time.sleep(5)

        MQTT_CLIENT.publish(MQTT_PUB_TOPIC_GEOFENCE, json_payload, 2)
        print("ho inviato: ", combined_item)
    
    else:
        pattern = r'\{"id": "(.*?)", "Latitude": "(.*?)", "Longitude": "(.*?)", "Satellites": "(.*?)", "l": "(.*?)", "s": "(.*?)"\}'

        match = re.search(pattern, r"{}".format(payload))

        if match:
            id_value = match.group(1)
            latitude_value = match.group(2)
            longitude_value = match.group(3)
            satellites_value = match.group(4)
            light_value = match.group(5)
            sound_value = match.group(6)

            print("ID:", id_value)
            print("Latitude:", latitude_value)
            print("Longitude:", longitude_value)
            print("Satellites:", satellites_value)
            print("Light:", light_value)
            print("Sound:", sound_value)

        else:
            print("No match found.")

        # Convert latitude and longitude to bytes
        latitude_bytes = bytes.fromhex(latitude_value)
        longitude_bytes = bytes.fromhex(longitude_value)

        # Add datetime information as the second field
        new_payload = {
            'id': id_value,
            'latitude': base64.b64encode(latitude_bytes).decode('utf-8'),
            'longitude': base64.b64encode(longitude_bytes).decode('utf-8'),
            'sat_num': satellites_value,
            'light': light_value,
            'sound': sound_value
        }

        json_payload = json.dumps(new_payload)

        # Topic will be MQTT_PUB_TOPIC_AIR
        topic = MQTT_PUB_TOPIC + "data/gps"

        success = myMQTTClient.publish(topic, json_payload, 0)

        time.sleep(5)
        if success:
            print("Message " + json_payload + " published to topic " + topic)
    print('-----')
    

# Set the callback function for MQTT messages on myMQTTClient
def on_my_message(_client, _userdata, message):
    """Handle messages received on myMQTTClient"""
    # Process the received message here
    payload = message.payload
    print("Received message on myMQTTClient:", payload)

    pattern = r'\["(.*?)","(.*?)"\]'

    match = re.search(pattern, payload.decode())

    if match:
        light = match.group(1)
        buzz = match.group(2)

        print("Light:", light)
        print("Buzz:", buzz)

        combined_item = [light, buzz]

        MQTT_CLIENT.publish(MQTT_SUB_TOPIC_CONTROL, str(combined_item), 2)
        print("ho inviato: ", combined_item)

    else:
        print("No match found.")


# On connect subscribe to topic
def on_connect(_client, _userdata, _flags, result):
    """Subscribe to input topic"""

    print('Connected ' + str(result))
#    myMQTTClient.publish(MQTT_PUB_TOPIC, "Ciao", 0)
#    print("ho inviato ciao")

    print('Subscribing to ' + MQTT_SUB_TOPIC)
    MQTT_CLIENT.subscribe(MQTT_SUB_TOPIC)

# Disconnect function
def disconnect_clients(signum, frame):
    MQTT_CLIENT.loop_stop()
    MQTT_CLIENT.disconnect()
    myMQTTClient.disconnect()
    print("\nDisconnected from clients")
    exit(0)

# Register signal handler for CTRL+C
signal.signal(signal.SIGINT, disconnect_clients)

# MQTT broker settings
MQTT_BROKER_ADDR = pyconfig.MQTT_BROKER_ADDR
MQTT_BROKER_PORT = 1883
MQTT_BROKER_CLIENT_ID = "broker"

# AWS IoT settings
AWS_IOT_ENDPOINT = pyconfig.AWS_IOT_ENDPOINT
AWS_IOT_PORT = 8883
AWS_IOT_CLIENT_ID = pyconfig.AWS_IOT_CLIENT_ID

# Set the relative path to the AWS IoT Root CA file
AWS_IOT_ROOT_CA = pyconfig.AWS_IOT_ROOT_CA

# Set the relative path to the AWS IoT Private Key file
AWS_IOT_PRIVATE_KEY = pyconfig.AWS_IOT_PRIVATE_KEY

# Set the relative path to the AWS IoT Certificate file
AWS_IOT_CERTIFICATE = pyconfig.AWS_IOT_CERTIFICATE

AWS_ACCESS_KEY_ID = pyconfig.AWS_ACCESS_KEY_ID

AWS_SECRET_ACCESS_KEY = pyconfig.AWS_SECRET_ACCESS_KEY


# For certificate based connection
myMQTTClient = AWSIoTMQTTClient(AWS_IOT_CLIENT_ID)

# Configurations
# For TLS mutual authentication
myMQTTClient.configureEndpoint(AWS_IOT_ENDPOINT, 8883)
myMQTTClient.configureCredentials(AWS_IOT_ROOT_CA, AWS_IOT_PRIVATE_KEY, AWS_IOT_CERTIFICATE)

myMQTTClient.configureOfflinePublishQueueing(-1)  # Infinite offline Publish queueing
myMQTTClient.configureDrainingFrequency(2)  # Draining: 2 Hz
myMQTTClient.configureConnectDisconnectTimeout(10)  # 10 sec
myMQTTClient.configureMQTTOperationTimeout(5)  # 5 sec

#TOPIC
MQTT_SUB_TOPIC = "sensor/gps"
MQTT_PUB_TOPIC = "device/"
MQTT_PUB_TOPIC_GEOFENCE = "geofence"
MQTT_SUB_TOPIC_CONTROL = "actuators/"

# Create a MQTT client instance
MQTT_CLIENT = mqtt.Client(client_id=MQTT_BROKER_CLIENT_ID)

# MQTT callback function
def main():
    MQTT_CLIENT.on_connect = on_connect
    MQTT_CLIENT.on_message = on_message
    MQTT_CLIENT.connect(MQTT_BROKER_ADDR, MQTT_BROKER_PORT)
    # Configure the callback function for myMQTTClient
    myMQTTClient.connect()
    myMQTTClient.subscribe(MQTT_SUB_TOPIC_CONTROL, 0, on_my_message)
    MQTT_CLIENT.loop_forever()

if __name__ == '__main__':
    main()
