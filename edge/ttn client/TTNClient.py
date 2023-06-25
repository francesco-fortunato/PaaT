import paho.mqtt.subscribe as subscribe
import paho.mqtt.publish as publish
import pyconfig
import boto3
import base64
import time

def on_message(client, userdata, message):
    global counter  # Use the global counter variable

    print(message.topic)
    payload_str = message.payload.decode('utf-8')  # Convert bytes to string
    print(payload_str)

    # Publish a new message
    if (message.topic == "v3/flamex@ttn/devices/eui-70b3d57ed005d56f/join"):
        dynamodb = boto3.resource('dynamodb',
                                aws_access_key_id = pyconfig.AWS_ACCESS_KEY_ID,
                                aws_secret_access_key = pyconfig.AWS_SECRET_ACCESS_KEY,
                                region_name = 'us-east-1')  # Replace with your desired region

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
        
        # combined item is [lat1, lng1, lat2, lng2, lat3, lng3, lat4, lng4].
        #take the extreme values, add an error of ca. 15 meters
        maxlat = str((max(combined_item[::2])) + 0.00015)
        maxlng = str((max(combined_item[1::2])) + 0.00015)
        minlat = str((min(combined_item[::2])) - 0.00015)
        minlng = str((min(combined_item[1::2])) - 0.00015)

        maxlat = maxlat[:15].ljust(15, '0')
        maxlng = maxlng[:15].ljust(15, '0')
        minlat = minlat[:15].ljust(15, '0')
        minlng = minlng[:15].ljust(15, '0')

        result = f"{maxlat},{maxlng},{minlat},{minlng}"
        print(result)

        publish.single(
            "v3/"+ pyconfig.USERNAME +"/devices/eui-70b3d57ed005d56f/down/replace",
            payload='{"downlinks":[{"f_port": 2,"frm_payload":"'+ base64.b64encode(result.encode()).decode() +'","priority": "HIGH"}]}',
            hostname="eu1.cloud.thethings.network",
            port=1883,
            auth={'username': pyconfig.USERNAME, 'password': pyconfig.PASSWORD}
        )

        print('ho pubblicato la geofence')
    
subscribe.callback(
    on_message,
    topics=['#'],
    hostname="eu1.cloud.thethings.network",
    port=1883,
    auth={'username': pyconfig.USERNAME, 'password': pyconfig.PASSWORD}
)
