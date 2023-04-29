#This guide is for Arduino IDE 2.0.4
## install arduino IDE
https://www.arduino.cc/en/software

## Install CH340 Driver
https://www.wemos.cc/en/latest/ch340_driver.html

Make sure Arduino Wemos D1 is plugged in via USB
## Installation of WeMos D1 board
Ctrl + Comma (,)
At the most bottom of Preferences,
Add additional Board URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json

Click the Board Drop down beside the check, right arrow
Select Other Board and port
Search for Wemos
Select LOLIN(WEMOS) D1 R2 & mini
and the COM port of the Arduino 
Click Ok

A popup on the lower right should pop up to install esp8266 [v 3.1.2]
click install manually

on the boards manager window, click Install

wait for it to install

``
Downloading packages
esp8266:xtensa-lx106-elf-gcc@3.1.0-gcc10.3-e5f9fec
esp8266:mkspiffs@3.1.0-gcc10.3-e5f9fec
esp8266:mklittlefs@3.1.0-gcc10.3-e5f9fec
esp8266:python3@3.7.2-post1
esp8266:esp8266@3.1.2
Installing esp8266:xtensa-lx106-elf-gcc@3.1.0-gcc10.3-e5f9fec
Configuring tool.
esp8266:xtensa-lx106-elf-gcc@3.1.0-gcc10.3-e5f9fec installed
Installing esp8266:mkspiffs@3.1.0-gcc10.3-e5f9fec
Configuring tool.
esp8266:mkspiffs@3.1.0-gcc10.3-e5f9fec installed
Installing esp8266:mklittlefs@3.1.0-gcc10.3-e5f9fec
Configuring tool.
esp8266:mklittlefs@3.1.0-gcc10.3-e5f9fec installed
Installing esp8266:python3@3.7.2-post1
Configuring tool.
esp8266:python3@3.7.2-post1 installed
Installing platform esp8266:esp8266@3.1.2
Configuring platform.
Platform esp8266:esp8266@3.1.2 installed
``

You can now try uploading the Blink Sketch to test
File > Examples > Basics > Blink
Upload to Arduino


## Install PubSubClient 2.8.0
Tools > Manage Libraries

Search for pubsubclient

PubSubClient by Nick O'Leary
Click Install

``
Downloading PubSubClient@2.8.0
PubSubClient@2.8.0
Installing PubSubClient@2.8.0
Installed PubSubClient@2.8.0
``


## Install ArduinoJson 6.21.2 by Benolt Blanchon
Tools > Manage Libraries

Search for ArduinoJson


``
Downloading ArduinoJson@6.21.2
ArduinoJson@6.21.2
Installing ArduinoJson@6.21.2
Installed ArduinoJson@6.21.2
``

## Install LiquidCrystal I2C 1.0.7 by Frank de Brabander

Tools > Manage Libraries

Search for LiquidCrystal I2C

Downloading LiquidCrystal I2C@1.1.2
LiquidCrystal I2C@1.1.2
Installing LiquidCrystal I2C@1.1.2
Installed LiquidCrystal I2C@1.1.2

## Install RTCLib 2.1.1 by Adafruit

Tools > Manage Libraries

Search for RTCLib

Downloading RTClib@2.1.1
RTClib@2.1.1
Installing RTClib@2.1.1
Installed RTClib@2.1.1
Downloading Adafruit BusIO@1.14.1
Adafruit BusIO@1.14.1
Installing Adafruit BusIO@1.14.1
Installed Adafruit BusIO@1.14.1

## Install PZEM004Tv30 by Jakub Mandula

Tools > Manage Libraries

Search for PZEM004Tv30

Downloading PZEM004Tv30@1.1.2
PZEM004Tv30@1.1.2
Installing PZEM004Tv30@1.1.2
Installed PZEM004Tv30@1.1.2

## Install PZEM004Tv30 by Jakub Mandula
https://github.com/xreef/PCF8574_library

Download Zip, extract to Documents/Arduino/libraries

## Pin connection

| NodeMCU | LCD |
|---------|-----|
| 5V      | VCC |
| GND     | GND |
| D3      | SCL |
| D4      | SDA |

| NodeMCU | DS1307 |
|---------|--------|
| GND     | GND    |
| 5V      | VCC    |
| D3      | SCL    |
| D4      | SDA    |

| NodeMCU | Relay |
|---------|-------|
| 5V      | VIN   |
| GND     | GND   |
| D0      | IN1   |
| D1      | IN2   |
| D2      | IN3   | 

| NodeMCU               | SD Card Module |
|-----------------------|----------------|
| D5                    | SCK            |
| D6                    | MOSO (sic)     |
| D7                    | MOSI           |
| D8                    | CS             |
| 5V (extension board)  | VCC            |
| GND (extension board) | GND            |