'''
    Device Simulator: publish messages sensors and subscribe relay comands
'''

import paho.mqtt.client as mqtt
import time
import json
import random

BROKER = "YOUR_MQTT_BROKER"
PORT = 1883
user = "YOUR_MQTT_USER"
pwd = "YOUR_MQTT_PASSWORD"
topic_pub_event = "wot/device_02/events/overheating"
topic_pub_temp = "wot/device_02/properties/temperature"
topic_pub_humd = "wot/device_02/properties/humidity"
topic_pub_relay = "wot/device_02/properties/relayState"
topic_sub_relay = "wot/device_02/actions/setRelay"

relay_state = "off"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected MQTT Broker")
        client.subscribe(topic_sub_relay)
        print(f"Subscribe topic: {topic_sub_relay}")
    else:
        print(f"Connection failure...")

def on_message(client, userdata, msg):
    print(f"Message received : topic = {msg.topic}, payload = {msg.payload.decode('utf-8')}")
    relay = msg.payload.decode('utf-8')
    
    try:
        data_relay = json.loads(relay)
        json_message_relay = json.dumps(data_relay)
    except json.JSONDecodeError:
        data_relay = relay
        json_message_relay = relay

    command = str(data_relay).lower().strip()
    
    if command == "on":
        relay_state = command
        print("[Relay] Start device")
    elif command == "off":
        relay_state = command
        print("[Relay] Stop device")
    else:
        print(f"Command not recognized: '{json_message_relay}'")
        return
    
    info = client.publish(topic_pub_relay, json_message_relay)
    info.wait_for_publish(timeout=1)

    if info.rc == mqtt.MQTT_ERR_SUCCESS:
        print(f"Publish: message = {json_message_relay}, topic = {topic_pub_relay}")
    else:
        print(f"Publish: failure = (rc={info.rc})")

client = mqtt.Client()
client.username_pw_set(user, pwd)

client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, keepalive=60)

client.loop_start()

try:
    random.seed(1,2)
    while (True):
        time.sleep(10)
        # Property: temperature
        temperature = round(random.uniform(0.0, 50.0), 1)
        json_message_temp = json.dumps(temperature)
        info = client.publish(topic_pub_temp, json_message_temp)
        info.wait_for_publish(timeout=1)	
        if info.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"Publish: message = {json_message_temp}, topic = {topic_pub_temp}")
        else:
            print(f"Publish: failure = (rc={info.rc})")
        # Event: Overheating
        if temperature > 40:
            json_message_event = json.dumps(temperature)
            info = client.publish(topic_pub_event, json_message_event)
            info.wait_for_publish(timeout=1)
            if info.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"Publish: message = {json_message_event}, topic = {topic_pub_event}")
            else:
                print(f"Publish: failure = (rc={info.rc})")
        # Property: humidity
        humidity = 100.0 - (temperature * 2.0)
        noise = random.uniform(-5.0, 5.0)
        humidity = round(humidity + noise, 1)
        json_message_humd = json.dumps(humidity)
        info = client.publish(topic_pub_humd, json_message_humd)
        info.wait_for_publish(timeout=1)
        if info.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"Publish: message = {json_message_humd}, topic = {topic_pub_humd}")
        else:
            print(f"Publish: failure = (rc={info.rc})")
except KeyboardInterrupt:
    print("\nFinishing MQTT client...")
    client.loop_stop()
    client.disconnect()
