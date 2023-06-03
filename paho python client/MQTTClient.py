from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
from decimal import Decimal
import paho.mqtt.client as mqtt
import json
import time
from datetime import datetime
import signal
import pyconfig
import boto3



# Set the callback function for MQTT messages
def on_message(_client, _userdata, message):
    """Handle messages"""

    print('-----')

    # Parse the incoming JSON string into a dictionary
    payload = json.loads(message.payload)

    # Add datetime information as the second field
    new_payload = {
        'id': payload['id'],
        'latitude': float(payload['latitude']),
        'longitude': float(payload['longitude']),
        'sat_num': int(payload['sat_num']),
        'last_movement': int(payload['last_movement']),
        'sound': bool(payload['sound'])
    }

    json_payload = json.dumps(new_payload)

    # Topic will be MQTT_PUB_TOPIC_AIR
    topic = MQTT_PUB_TOPIC + payload['id'] + "/data"

    success = myMQTTClient.publish(topic, json_payload, 0)

    time.sleep(5)
    if (success):
        print("Message published to topic "+ topic)
    print('-----')
    
# On connect subscribe to topic
def on_connect(_client, _userdata, _flags, result):
    """Subscribe to input topic"""

    print('Connected ' + str(result))
#    myMQTTClient.publish(MQTT_PUB_TOPIC, "Ciao", 0)
#    print("ho inviato ciao")

    print('Subscribing to ' + MQTT_SUB_TOPIC)
    MQTT_CLIENT.subscribe(MQTT_SUB_TOPIC)

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

    MQTT_CLIENT.publish(MQTT_PUB_TOPIC_GEOFENCE, json_payload, 2)
    print("ho inviato: ", combined_item)

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

# Create a MQTT client instance
MQTT_CLIENT = mqtt.Client(client_id=MQTT_BROKER_CLIENT_ID)

# MQTT callback function
def main():
    MQTT_CLIENT.on_connect = on_connect
    MQTT_CLIENT.on_message = on_message
    MQTT_CLIENT.connect(MQTT_BROKER_ADDR, MQTT_BROKER_PORT)
    myMQTTClient.connect()
    MQTT_CLIENT.loop_forever()

if __name__ == '__main__':
    main()
