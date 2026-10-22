'''
    Application publish messages to Bob agent via terminal
'''

import paho.mqtt.client as mqtt
import time
import json

BROKER = "YOUR_MQTT_BROKER"
PORT = 1883
user = "YOUR_MQTT_USER"
pwd = "YOUR_MQTT_PASSWORD"
TOPIC_PUBLISH = "wot/agents/bob"
TOPIC_SUBSCRIBE = "wot/device_02/properties/relayState"

# Callback: conexão estabelecida com o broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected MQTT Broker")
        client.subscribe(TOPIC_SUBSCRIBE)
        print(f"Subscribe topic: {TOPIC_SUBSCRIBE}")
    else:
        print(f"Conection failure...")

def on_message(client, userdata, msg):
    print(f"Message received: topic = {msg.topic}, payload = {msg.payload.decode('utf-8')}")

client = mqtt.Client()
client.username_pw_set(user, pwd)

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)

client.loop_start()

try:
    while (True):
        time.sleep(2)
        # Messages JSON to Bob: device_01 setLed on | device_02 setRelay on
        target, operation, data = input("Enter request to Bob: ").split() #request(Target, Operation, Data)
        message = {"target":target,"operation":operation, "data":data}
        json_message = json.dumps(message)
        info = client.publish(TOPIC_PUBLISH, json_message)
        info.wait_for_publish(timeout=1)
        if info.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"Publish: message = {json_message}, topic = {TOPIC_PUBLISH}")
        else:
            print(f"Publish: failure = (rc={info.rc})")
except KeyboardInterrupt:
    print("\nFinishing MQTT client...")
    client.loop_stop()
    client.disconnect()
