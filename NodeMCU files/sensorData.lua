SDA_PIN = 2 -- sda pin, GPIO4
SCL_PIN = 1 -- scl pin, GPIO5

si7021 = require("si7021")
si7021.init(SDA_PIN, SCL_PIN)   --  Setting the i2c pin of si7021
si7021.read(OSS)

Hum = si7021.getHumidity()      -- Get humidity from si7021
Temp = si7021.getTemperature()  -- Get temperature from si7021

Hum = Hum / 100 .. "." .. Hum % 100 .. " %"
Temp = Temp / 100 .. "." .. (Temp % 100) .. " C"

-- pressure in differents units
print("Humidity: ".. Hum)

-- temperature in degrees Celsius
print("Temperature: ".. Temp)

-- release module
si7021 = nil
package.loaded["si7021"]=nil
