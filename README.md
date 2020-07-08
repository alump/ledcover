# Seeedstudio Odyssey Led Ring Cover Arduino/PlatformIO project

Made this code to control the LED rings attached to 3D-printed cover of Seeedstudio's case for Odyssey.
It's to be run on SAMD21 MCU on Odyssey. And can be then controlled by Linux, if you attach
the I2C pins between "Rasberry PI" and SAMD21 headers.

Some shell scripts how to command it on usage folder. Notice that if you use other I2C port, you need to change that. And also if you change the I2C address, you need to change those.

```
CONFIGURATION: https://docs.platformio.org/page/boards/atmelsam/seeed_zero.html
PLATFORM: Atmel SAM 4.5.1 > Seeeduino Zero
HARDWARE: SAMD21G18A 48MHz, 32KB RAM, 256KB Flash
```