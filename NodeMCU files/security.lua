-- Global variables and parameters.
nodeID = "001"	            -- a node identifier for this device
tgtHost = "192.168.1.11"	-- target host (broker)
tgtPort = 1883			    -- target port (broker listening on)
mqttUserID = "mosquitto"	-- account to use to log into the broker
mqttPass = "password"		-- broker account password
mqttTimeOut = 120		    -- connection timeout
dataInt = 1			        -- data transmission interval in seconds 
topic1 = "/system_name/" .. nodeID .. "/" .. "config"   -- the MQTT topic queue to use
topic2 = "/system_name/" .. nodeID .. "/" .. "action" 
topic3 = "/system_name/" .. "discover"
    
-- Reconnect to MQTT when we receive an "offline" message.
  
function reconn()
    print("Disconnected, reconnecting....")
    conn()
end  
   
-- Establish a connection to the MQTT broker with the configured parameters.
     
function conn()
    print("Making connection to MQTT broker")
    mqttBroker:connect(tgtHost, tgtPort, 0, function(client) print ("connected")
    mqttSub() end, function(client, reason) print("failed reason: "..reason) end)
end

function mqttSub()

    mqttBroker:subscribe(topic1, 0, function(conn)
        print("Subscribing topic: " .. topic1)
        
        mqttBroker:subscribe(topic2, 0, function(conn)
            print("Subscribing topic: " .. topic2)
            main()
        end)
    end)
end

-- Function pubEvent() publishes the sensor value to the defined queue.
     
function pubEvent(pubValue, topicQueue)
    -- rv = adc.read(0)                -- read temp sensor
    --pubValue = nodeId .. " " .. 24        -- build buffer
    print("Publishing to " .. topicQueue .. ": " .. pubValue)   -- print a status message
    mqttBroker:publish(topicQueue, pubValue, 0, 0)  -- publish
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
    
    -- Connect to the Broker
    conn()
end

function main()
    discover = nodeID
    
    pubEvent(discover, topic3)

end
