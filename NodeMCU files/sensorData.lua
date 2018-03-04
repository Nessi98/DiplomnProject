    SDA_PIN = 2-- sda pin, GPIO4
    SCL_PIN = 1 -- scl pin, GPIO5

    si7021 = require("si7021")
    si7021.init(SDA_PIN, SCL_PIN)
    si7021.read(OSS)
    Hum = si7021.getHumidity()
    Temp = si7021.getTemperature()
    -- pressure in differents units
    print("Humidity: ".. (Hum / 100) .. "." .. (Hum % 100) .. " %")

    -- temperature in degrees Celsius
    print("Temperature: "..(Temp / 100).."."..(Temp % 100).." deg C")

    -- release module
    si7021 = nil
    package.loaded["si7021"]=nil
