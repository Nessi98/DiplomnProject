SSID    = "Joro"
APPWD   = "Vanesa98"
CMDFILE = "ping.lua"   -- File that is executed after connection

wifiTrys     = 15     -- Counter of trys to connect to wifi
NUMWIFITRYS  = 200    -- Maximum number of WIFI Testings while waiting for connection

function launch()
    print("Connected to WIFI!")
    print("IP Address: " .. wifi.sta.getip())
  
    -- Call command file
    dofile("security.lua")
    makeConn()
end

function checkWIFI() 
    if ( wifiTrys > NUMWIFITRYS ) then
        print("Sorry. Not able to connect")
    else
        ipAddr = wifi.sta.getip()
        if ( ( ipAddr ~= nil ) and  ( ipAddr ~= "0.0.0.0" ) )then
            tmr.alarm( 1 , 500 , 0 , launch )
        else
            -- Reset alarm again
            tmr.alarm( 0 , 2500 , 0 , checkWIFI)
            print("Checking WIFI..." .. wifiTrys)
            wifiTrys = wifiTrys + 1
        end 
    end 
end


-- See if we are already connected by getting the IP
ipAddr = wifi.sta.getip()

if ( ( ipAddr == nil ) or  ( ipAddr == "0.0.0.0" ) ) then
    -- Not connected, so let's connect
    print("Configuring WIFI....")
    wifi.setmode( wifi.STATION )
    wifi.sta.config( SSID , APPWD)
    print("Waiting for connection")
    tmr.alarm( 0 , 2500 , 0 , checkWIFI )
else
    -- Connected, so just run the launch code.
    launch()
end
