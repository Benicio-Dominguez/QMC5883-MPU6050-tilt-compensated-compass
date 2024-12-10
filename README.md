# QMC5883-MPU6050-tilt-compensated-compass
Code and schematic for building a precise tilt compensated compass using QMC5883 and MPU6050 (or a MPU9050 knockoff with no magnetometer).
Works nicely on ESP32, should work on Arduino as well. It has a compass calibration algorithm.

The only connection is the following:
* MPU and QMC are connected to the I2C bus. If you are using an ESP32, a logical level shifter is necessary.

For the tilt compensation to work right off, the Z axis of both sensors must coincide, the X axis of the QMC must point in the same direction as the Y axis of the MPU, and the Y axis of the QMC must point in the opposite direction of the X axis of the MPU. If you want to place the sensors in a different way, you must change a little the code.

"Gire el magnetómetro..." means that you have to turn the sensor around its axes slowly and at a constant pace, it is the magnetometer calibration.

Important: The code depends on "MPU9250_WE" and "Mecha_QMC5883L" libraries. 
Links: 

https://github.com/wollewald/MPU9250_WE

https://github.com/keepworking/Mecha_QMC5883L

Just download and put them on your arduino libraries directory.

Good luck, contact me at beniciodomi@gmail.com if you need more information or help.
