-- Global variables and parameters.
nodeID = 4	            -- a node identifier for this device
sleepTime = 1200        -- duration of the energy saving mode

-- Variables for connection to the Brocker
tgtHost = "192.168.1.12"	-- target host (broker)
tgtPort = 1883			    -- target port (broker listening on)
mqttUserID = "mosquitto"	-- account to use to log into the broker
mqttPass = "password"		-- broker account password
mqttTimeOut = 120		    -- connection timeout		
messageReceive = 1          -- data transmission interval in seconds 

-- Topic variables
configTopic = "/system_name/" .. nodeID .. "/" .. "config" 
configAckTopic = "/system_name/" .. nodeID .. "/config_ack"
discoverTopic = "/system_name/" .. "discover"

-- Op_Mode variables
opMode = 2
modeIDLE = "IDLE"
modeSensor = "Sensor"
modeRelay = "Relay"
modeSensorAndRelay = "Sensor and Relay"

--Op_mode parameters
IDLE = 1
Senosr = 2
Relay = 3
SensorAndRelay = 4 

SDA_PIN = 2 -- sda pin, GPIO4
SCL_PIN = 1 -- scl pin, GPIO5
    
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
        function(conn) print("Subscribe for " .. configTopic .. " successful")
        main()
        end)
end


-- Function pubEvent() publishes the data to the defined queue.   
function pubEvent(message, topic)
    -- Print a status message
    print("Publishing to " .. topic .. ": " .. message)
    mqttBroker:publish(topic, message, 0, 0)  -- publish
end

function getData()

    si7021 = require("si7021")
    si7021.init(SDA_PIN, SCL_PIN)   --  Setting the i2c pin of si7021
    si7021.read(OSS)
        
    Hum = si7021.getHumidity()      -- Get humidity from si7021
    Temp = si7021.getTemperature()  -- Get temperature from si7021
    
    Hum = Hum / 100 .. "." .. Hum % 100 
    Temp = Temp / 100 .. "." .. (Temp % 100)
    
    -- pressure in differents units
    print("Humidity: ".. Hum)
    
    -- temperature in degrees Celsius
    print("Temperature: ".. Temp)
    data = Temp .. "," .. Hum
    -- release module
    si7021 = nil
    package.loaded["si7021"] = nil

end

function wake()
    --conect to wrifi
    --conect to brocker 
    -- if mode = sensor or senro and relay -> get data

    --if opMode == Sensor or opMode == SensorAndRelay then
    getData()
    --end
end

function messageArrieved(client, topic, data)
    print("Delivered message on topic" .. topic .. ":" ) 
    wake()
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
    
    print("Waked")
    
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
    --while (messageReceive > 0)
    --do
        pubEvent(nodeID, discoverTopic)
        --pubEvent("24,38%,2018/11/24 15:45:35", "/system_name/1/data")
        messageReceive = 0
    --end
end