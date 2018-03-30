-- Global variables and parameters.
nodeID = "001"	            -- a node identifier for this device
tgtHost = "192.168.1.13"	-- target host (broker)
tgtPort = 1883			    -- target port (broker listening on)
mqttUserID = "mosquitto"	-- account to use to log into the broker
mqttPass = "password"		-- broker account password
mqttTimeOut = 120		    -- connection timeout
dataInt = 1			
messageReceive = 1          -- data transmission interval in seconds 
configTopic = "/system_name/" .. nodeID .. "/" .. "config"   -- the MQTT topic queue to use 
discoverTopic = "/system_name/" .. "discover"
    
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
     
-- makeConn() instantiates the MQTT control object, sets up callbacks,
-- connects to the broker, and then uses the timer to send sensor data.
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
    mqttBroker:on("message", function(client, topic, data) 
        print(topic .. ":" ) 
        if data ~= nil then
            print(data)
            messageReceive = 0
        end
    end)
    
    -- Connect to the Broker
    conn()
end

function main()    
    repeat
        pubEvent(nodeID, discoverTopic)
    until messageReceive ~= 0
end
