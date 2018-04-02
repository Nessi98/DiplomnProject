-- Global variables and parameters.
nodeID = "001"	            -- a node identifier for this device
sleepTime = 0               -- duration of the energy saving mode

-- Variables for connection to the Brocker
tgtHost = "192.168.1.12"	-- target host (broker)
tgtPort = 1883			    -- target port (broker listening on)
mqttUserID = "mosquitto"	-- account to use to log into the broker
mqttPass = "password"		-- broker account password
mqttTimeOut = 120		    -- connection timeout		
messageReceive = 1          -- data transmission interval in seconds 

-- Topic variables
configTopic = "/system_name/" .. nodeID .. "/" .. "config" 
discoverTopic = "/system_name/" .. "discover"

-- Op_Mode variables
opMode = 0 
modeIDLE = "IDLE"
modeSensor = "Sensor"
modeRelay = "Relay"
modeSensorAndRelay = "Sensor and Relay"

--Op_mode parameters
IDLE = 1
Senosr = 2
Relay = 3
SensorAndRelay = 4 
    
-- Reconnect to MQTT when we receive an "offline" message.
  
function reconn()
    print("Disconnected, reconnecting....")
    conn()
end  
   
-- Establish a connection to the MQTT broker with the configured parameters.    
function conn()
    print("Making connection to MQTT broker")
    mqttBroker:connect(tgtHost, tgtPort, 0, function(client) print ("Connected")
    mqttSub() end, function(client, reason) print("failed reason: "..reason) end)
end

function mqttSub()
    mqttBroker:subscribe(configTopic, 0,
        function(conn) print("Subscribe success")
        main()
        end)
end


-- Function pubEvent() publishes the data to the defined queue.   
function pubEvent(message, topic)
    -- Print a status message
    print("Publishing to " .. topic .. ": " .. message)
    mqttBroker:publish(topic, message, 0, 0)  -- publish
end

function messageArrieved(client, topic, data)
    print("Delivered message on topic" .. topic .. ":" ) 
    
    if data ~= nil then
        print(data)
        messageReceive = 0
    end

    sleepTime = string.match(data, "%d+")
       
    if string.find(data, modeIDLE) then
        opMode = IDLE
    elseif (string.find(data, modeSensor))
    then
        -- set op_Mode to sensor
        opMode = Sensor
    elseif (string.find(data, modeRelay)) 
    then
        -- set op_Mode to Relay
        opMode = Relay
        -- get the value for the Relay
    elseif (string.find(data, modeSensorAndRelay)) 
    then
        -- set op_mode to Sensor and Relay
        opMode = SensorAndRelay
        -- get the value for the Relay
    else
        --nothing
    end
end
     
-- makeConn() instantiates the MQTT control object, sets up callbacks,
-- This is the "main" function in this library. This should be called 
-- from init.lua (which runs on the ESP8266 at boot), but only after
-- it's been vigorously debugged. 
  
function makeConn()
    -- Instantiate a global MQTT client object
    print("Instantiating mqttBroker")
    mqttBroker = mqtt.Client(nodeId, mqttTimeOut, mqttUserID, mqttPass, 1)
     
    -- Set up the event callbacks
    print("Setting up callbacks")
    mqttBroker:on("connect", function(client) print ("connected") end)
    mqttBroker:on("offline", reconn)
    
    mqttBroker:on("message", messageArrieved)
    
    -- Connect to the Broker
    conn()
end

function main()    
    while (messageReceive > 0)
    do
        pubEvent(nodeID, discoverTopic)
        messageReceive = 0
    end
end
