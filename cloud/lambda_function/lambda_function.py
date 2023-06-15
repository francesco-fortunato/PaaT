import json
import base64
import boto3
import time
from Crypto.Cipher import AES

def lambda_handler(event, context):
    # Extract the latitude and longitude from the input message
    latitude_base64 = event['latitude']
    longitude_base64 = event['longitude']

    # Decode the latitude and longitude from Base64
    latitude_encrypted = base64.b64decode(latitude_base64)
    longitude_encrypted = base64.b64decode(longitude_base64)

    # AES encryption/decryption settings
    key = b''  # Replace with your AES key
    cipher = AES.new(key, AES.MODE_ECB)

    # Decrypt the latitude and longitude
    latitude_decrypted = cipher.decrypt(latitude_encrypted).rstrip(b'\0').decode('utf-8')
    longitude_decrypted = cipher.decrypt(longitude_encrypted).rstrip(b'\0').decode('utf-8')

    # Print the decrypted latitude and longitude
    print("Decrypted latitude:", latitude_decrypted)
    print("Decrypted longitude:", longitude_decrypted)

    # Create a DynamoDB client
    dynamodb = boto3.resource('dynamodb')

    # Specify the table name
    table_name = 'paat_db'

    # Get the DynamoDB table
    table = dynamodb.Table(table_name)

    # Prepare the item to be saved in DynamoDB
    item = {
        'device_id': int(event['id']),
        'sample_time': int(time.time() * 1000),
        'Latitude': latitude_decrypted,
        'Longitude': longitude_decrypted
    }

    # Save the item in DynamoDB
    table.put_item(Item=item)

    return {
        'statusCode': 200,
        'body': json.dumps('Decryption completed and values saved in DynamoDB!')
    }

