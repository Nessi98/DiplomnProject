import paho.mqtt.client as mqtt
import time
import json

port = 1883
clientID = "raspi"
broker = "127.0.0.1"
mqttclient_log = True
topic = "system_name/"
# if not connected after this time assume failure
max_connection_time = 6

inputs={"broker":broker,"port":port,"topic": topic + "discover","loops":\
        1,"loop_delay":1,"silent_flag":False,"username":"mosquitto",\
        "password":"password"}
mqttclient_log = False

def on_connect(client, userdata, flags, rc):
    if rc==0:
        client.connected_flag = True
        print("Connected success!")
    else:
        client.bad_connection_flag=True
        if rc==5:
            print("Broker requires authentication.")
        

def on_disconnect(client, userdata, rc):
    m = "Disconnecting reason: " ,str(rc)
    client.connect_flag=False
    client.disconnect_flag=True
    
def on_subscribe(client, userdata, mid, granted_qos):
    print("Subscribed successed.")
    client.suback_flag = True
	
def on_publish(client, userdata, mid):
    client.puback_flag = True

def on_message(client, userdata, message):
    topic = message.topic
    msgr = str(message.payload.decode("utf-8"))
    responses.append(json.loads(msgr))
    client.rmsg_count += 1
    client.rmsg_flagset = True

def on_log(client, userdata, level, buf):
    print("log: ",buf)

def Initialise_client_object():  
    mqtt.Client.bad_connection_flag = False
    mqtt.Client.suback_flag = False
    mqtt.Client.connected_flag = False
    mqtt.Client.disconnect_flag = False
    mqtt.Client.disconnect_time = 0.0
    mqtt.Client.disconnect_flagset = False
    mqtt.Client.rmsg_flagset = False
    mqtt.Client.rmsg_count = 0
    mqtt.Client.display_msg_count = 0

def Initialise_clients(clientID):
    #flags set
    client = mqtt.Client(clientID)
    if mqttclient_log: #enable mqqt client logging
        client.on_log = on_log
    client.on_connect = on_connect        #attach function to callback
    client.on_message = on_message        #attach function to callback
    client.on_disconnect = on_disconnect
    client.on_subscribe = on_subscribe
    client.on_publish = on_publish
    return client 

#Start 
if __name__ == "__main__":

    import sys, getopt
    if len(sys.argv)>=2:
        get_input(sys.argv[1:])


Initialise_client_object()

#Create client object and set callbacks
client = Initialise_clients(clientID)

if inputs["username"]!= "": #set username/password
    client.username_pw_set(username = inputs["username"],password = inputs["password"])

print("Connecting to broker...")
try:
	#Establish connection
    res = client.connect(inputs["broker"],inputs["port"])           
except:
    print("Can't connect to broker.")
    sys.exit()

client.loop_start()
tstart = time.time()

while not client.connected_flag and not client.bad_connection_flag:
    time.sleep(.25)
if client.bad_connection_flag:
    print("Connection failure to broker.")
    sys.exit()

if inputs["silent_flag"]:
    print ("Silent Mode is on")  
	
client.subscribe(inputs["topic"])
#Wait for subscribe to be acknowledged
while not client.suback_flag: 
    time.sleep(.25)

try:
	while client.suback_flag:
		print("Something")
	
except KeyboardInterrupt:
    print("Interrrupted by keyboard.")

time.sleep(2)
client.disconnect()
client.loop_stop()
time.sleep(2)