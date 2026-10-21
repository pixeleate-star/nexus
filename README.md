# NEXUS ESP32 Smart Dashboard

this is the whole codebase for my nexus esp32 dashboard it has the code circuit connections and setup info you need to build your own compact smart dashboard using an esp32 c3 super mini st7735 tft display mpu6050 motion sensor and rotary encoder

the main idea behind this project is to build a small interactive device that combine different useful features into one system instead of making separate projects for motion sensing system information wifi connectivity and bluetooth controls i wanted to bring everything together in one dashboard

the esp32 c3 handles the main logic reads the sensors manages user input and runs the different features the tft display gives the interface while the rotary encoder make it easy to move thru the menus

as the project developed i also added a web dashboard bluetooth media controls a motion controlled space shooter game and an animated screensaver

<img width="1200" height="1600" alt="image" src="https://github.com/user-attachments/assets/d51c27ee-c477-4828-a56d-e0552df9162a" />


## Features

- compact esp32 c3 based dashboard
- st7735 tft display interface
- main menu with multiple features
- rotary encoder based navigation
- mpu6050 accelerometer and gyroscope
- real time motion sensor readings
- system information screen
- dedicated encoder testing screen
- wifi network scanning
- display of saved wifi networks
- wifi signal strength indication using rssi
- connect to available saved networks
- wifi connection status display
- web dashboard
- bluetooth media controls
- volume control using bluetooth
- media play and pause controls
- motion controlled space shooter game
- gyroscope calibration for motion based controls
- animated nexus screensaver
- automatic screensaver after inactivity
- wake the dashboard using the rotary encoder
- multiple features integrated into one device
- compact and easy to expand design

## BOM Bill of Materials

| component | quantity | purpose | purchase link |
|-----------|:--------:|---------|:-------------:|
| esp32 c3 super mini | 1 | main controller and processing | [buy here](https://www.google.com/search?q=ESP32+C3+Super+Mini) |
| st7735 or st7735s tft display | 1 | dashboard interface and graphics | [buy here](https://www.google.com/search?q=ST7735+TFT+display) |
| mpu6050 module | 1 | accelerometer and gyroscope for motion sensing | [buy here](https://www.google.com/search?q=MPU6050+module) |
| rotary encoder module | 1 | menu navigation and user input | [buy here](https://www.google.com/search?q=rotary+encoder+module) |
| jumper wires | as required | electrical connections | none |
| usb cable | 1 | programming and powering the esp32 | none |
| breadboard | 1 | prototyping and testing connections | none |
| enclosure | optional | protects the electronics and improves the final design | self designed |

the exact components can vary depending on the modules you have available the display and encoder connections should be checked against your particular hardware before powering everything on

## Circuit Diagram

i have used the following pin connections for the main nexus hardware you can use them as reference while connecting your own components

### Main Connections

<img width="1218" height="986" alt="image" src="https://github.com/user-attachments/assets/9af6f5ab-0eef-4d55-a828-958961512af3" />

i know its a mess so here a link for a clear detailed wiring of components  https://app.cirkitdesigner.com/project/a8816a45-51b7-4a8f-b14c-1ef5eea4e9e8

ST7735 TFT Display to ESP32 C3

| tft display | esp32 c3 |
|-------------|----------|
| rst | gpio 3 |
| cs | gpio 7 |
| dc | gpio 2 |
| mosi | gpio 6 |
| sclk | gpio 4 |
| vcc | suitable supply according to the display module |
| gnd | gnd |

MPU6050 to ESP32 C3

| mpu6050 | esp32 c3 |
|---------|----------|
| vcc | 3.3v |
| gnd | gnd |
| sda | gpio 1 |
| scl | gpio 5 |

the mpu6050 communicate with the esp32 using i2c its usual i2c address in this setup is 0x68

Rotary Encoder to ESP32 C3

| rotary encoder | esp32 c3 |
|----------------|----------|
| clk | gpio 0 |
| dt | gpio 10 |
| sw | gpio 20 |
| gnd | gnd |
| + | suitable supply according to the module |

the encoder is used to move thru the menus select options and interact with the different dashboard features

the exact power connections depends on the modules being used check the voltage requirements of the tft display and encoder before connecting them all the components should share a common ground

## How to Build This

step 1 first get all the components mentioned in the bom the main parts required are the esp32 c3 super mini st7735 tft display mpu6050 and rotary encoder

step 2 connect the tft display to the esp32 using the pin connections mentioned above make sure the display pins match the configuration in the code

step 3 connect the mpu6050 to the esp32 connect sda to gpio 1 and scl to gpio 5 connect vcc to 3.3v and gnd to gnd

step 4 connect the rotary encoder to gpio 0 gpio 10 and gpio 20 these pins are used for rotation detection and button input

step 5 connect the esp32 to your computer using a usb cable make sure board is detected properly before uploading the code

step 6 open arduino ide and install the required libraries the project use the adafruit st7735 display library for the tft interface install any additional libraries required by the particular source files you are using

step 7 select the correct esp32 c3 board and port in arduino ide upload the code and open the serial monitor if you need to check startup messages or debug the device

step 8 once the device starts check whether display is working properly test the rotary encoder and make sure the menu changes when you rotate or press it

step 9 open the motion sensor screen and check if the mpu6050 readings change when you move the device if the readings stays fixed check the i2c wiring and sensor address

step 10 after the basic hardware is working test the wifi manager web dashboard and bluetooth media controls some features needs additional setup on the connected computer or network

step 11 finally test the motion controlled game and screensaver once everything works properly you can assemble the components into a compact enclosure

## Main Menu

the main menu is the starting point of the nexus dashboard i wanted the interface to be simple enough to use without needing a keyboard or mouse the rotary encoder gives the main input so you can move thru the available options and select the feature you want

the main menu includes

- motion sensor
- system info
- encoder test
- wifi connect
- web dashboard
- mac media
- space shooter

each option opens a separate screen with its own functionality the menu structure also make it easier to add new features without changing the entire interface

## Motion Sensor

the motion sensor screen uses the mpu6050 to read movement data from the device the mpu6050 contains an accelerometer and a gyroscope the accelerometer measures acceleration along different axes while the gyroscope measures angular velocity these readings can be used to understand how the device is moving or rotating

the motion sensor is also useful for the motion controlled space shooter game where the movement of device is used as an input

the main things to check while testing the sensor are

- whether the sensor is detected correctly
- whether the readings change when the device moves
- whether the readings remain stable when the device is still
- whether the sensor orientation matches the expected controls

the sensor can also be used as a starting point for future projects involving gesture control orientation tracking and motion based interaction

## System Information

the system information screen gives a place to display useful information about the device instead of connecting the esp32 to a computer every time i want to check something the idea is to make important system information available directly on the tft display

this screen can also be expanded later to include more information such as wifi status uptime memory usage and other available system statistics

## Encoder Test

the encoder test screen is used to check whether the rotary encoder is working correctly the rotary encoder detects rotation using its clk and dt pins while the sw pin is used for button input

this screen is useful during development because it helps find problems with the encoder before using it to navigate thru the entire dashboard if the direction feels reversed the encoder logic can be adjusted in the code

## Wifi Manager

the wifi manager allows the device to scan for wireless networks and connect to saved networks instead of showing every available network without any filtering the interface checks which saved networks are currently visible the available information includes the network name and signal strength

the wifi manager includes

- scanning for available wifi networks
- finding saved networks that are currently available
- displaying signal strength using rssi
- selecting a network to connect to
- showing the connection status
- returning to the previous menu
- rescanning for networks

the connection process also handles different states such as connecting connected failed and cancelled

during testing i found that the network environment can make a difference a personal hotspot worked while the campus wifi did not connect successfully in my testing this showed that the problem is not always in the esp32 code network restrictions authentication requirements and access point configuration can also affect connectivity

## Web Dashboard

the web dashboard is another part of nexus that extends the project beyond the small tft display the idea is to make the device accessible thru a browser giving another way to interact with or view information from the system the esp32 networking functionality provide the foundation for connecting the device to a web based interface

when testing this feature make sure the esp32 and the device opening the dashboard are connected to a network that allow them to communicate with each other the exact dashboard address and available controls depends on the web server implementation in the source code

this feature also gives a foundation for adding more controls system information and interactive elements in future

## Bluetooth Media Controls

i also added bluetooth media controls to nexus so that it can interact with a computer without needing a physical keyboard the bluetooth device use the name nexus and supports media related controls thru bluetooth hid

the main idea is to control media playback and volume using the dashboard the feature includes

- bluetooth hid functionality
- media play and pause controls
- volume control
- wireless interaction with a compatible computer

during setup make sure bluetooth is enabled on the computer and pair with the nexus device when it appears bluetooth behavior can vary depending on the operating system and the way the computer handles media keys

this feature makes the project more useful as a desk gadget instead of being only a sensor dashboard

## Motion Controlled Space Shooter

one of the more interesting features of nexus is the motion controlled space shooter game instead of controlling the game using regular keyboard buttons the idea is to use the movement of the device as the input the mpu6050 gives the motion data which can be used to control movement within the game

the game also includes different states such as calibration shielding and game over calibration is important because the sensor should set a suitable reference position before the controls are used

the main things to check while testing the game are

- whether the sensor readings respond correctly to movement
- whether the controls feel natural
- whether the game responds consistently
- whether calibration sets the correct reference position
- whether the game over and other states work correctly

this feature was a fun way to use the motion sensor for something interactive instead of only displaying sensor readings

## Idle Screensaver

i also added an animated screensaver to make the dashboard feel more complete when the device stay inactive for around 30 seconds the screensaver can start automatically the screensaver includes an animated star field and a cyan and blue nexus logo

the rotary encoder can be used to wake the dashboard and return to the normal interface the screensaver is designed to avoid interrupting activities where the device is still being used such as gameplay calibration or an active wifi connection process

this small addition makes the interface feel more like a finished device rather than a collection of separate test screens

## Code

the main project uses the following components and concepts

- arduino framework for the esp32 c3
- tft display library for the st7735 screen
- i2c communication using wire.h
- mpu6050 accelerometer and gyroscope
- rotary encoder input handling
- wifi scanning and connection management
- web based dashboard functionality
- bluetooth hid media controls
- motion based game controls
- animated screensaver

the pin configuration used by the project is

```cpp
#define TFT_RST 3
#define TFT_CS 7
#define TFT_DC 2
#define TFT_MOSI 6
#define TFT_SCLK 4

#define SDA_PIN 1
#define SCL_PIN 5

#define ENCODER_CLK 0
#define ENCODER_DT 10
#define ENCODER_SW 20
```

these values should match the actual wiring of the hardware if you change the display encoder or sensor connections update the corresponding pin definitions in the code the libraries and configuration options required by individual features should be checked in their respective source files

## Known Issues

1. wifi connectivity can depend on the network configuration and authentication method
2. some campus networks may restrict connections from embedded devices
3. the tft display may require different initialization settings depending on the exact st7735 module
4. incorrect display pin connections can cause a blank screen or incorrect output
5. the rotary encoder may require adjustment if the direction or button input behave incorrectly
6. mpu6050 readings can fluctuate because of sensor noise and physical movement
7. sensor orientation affects how motion data should be interpreted
8. bluetooth pairing and media controls can behave differently across operating systems
9. the web dashboard requires a suitable network connection between the esp32 and the connected device
10. the screensaver timeout and wake behavior should be tested alongside the other dashboard features
11. combining multiple features can introduce timing or resource conflicts
12. additional features may require changes to the menu structure and input handling

## CAD Models

<img width="1906" height="1472" alt="image" src="https://github.com/user-attachments/assets/3110e894-ef55-472d-99f3-216b91ff4012" />
<img width="1702" height="1262" alt="image" src="https://github.com/user-attachments/assets/44f1454f-1951-4e28-966f-ee3e64516583" />
<img width="2278" height="1558" alt="image" src="https://github.com/user-attachments/assets/ed6c3016-9312-485f-ad38-2036c4499b73" />


## Working Principle

the basic idea behind nexus is to use the esp32 c3 as the main controller and combine multiple features into one compact system the display gives the interface the rotary encoder handles user input and the mpu6050 supplies motion data the wifi and bluetooth functionality provide wireless features while the web dashboard offers another way to interact with the device

the overall structure looks something like this

```text
              esp32 c3
                  |
       +----------+----------+
       |          |          |
    display     encoder    mpu6050
       |          |          |
       +----------+----------+
                  |
             main menu
                  |
       +----------+----------+
       |          |          |
      wifi     bluetooth    motion
       |          |          |
   web dashboard media      game
                  controls
                  |
             screensaver
```

the esp32 continuously handles the logic required by the active feature and updates the interface when needed when the user rotates or presses the encoder the input is processed by the menu system selecting a feature opens the corresponding screen or starts the relevant functionality

the motion sensor gives data for motion related features while wifi and bluetooth enable wireless interaction all these parts works together to turn a small esp32 board into a multi feature dashboard

## Final Result

the whole idea behind nexus was to build something more useful than a basic sensor display i started with a small esp32 c3 a tft display an mpu6050 and a rotary encoder from there i gradually brought different features together into one device

the project now combines menu navigation motion sensing wifi connectivity a web dashboard bluetooth media controls a motion controlled game and an animated screensaver it was also a good way to learn more about embedded programming sensor integration display interfaces wireless communication and combining multiple features without making the user interface difficult to navigate

there is still room to improve the design add more functionality and build a proper enclosure but the main goal was to create a compact device that bring several useful ideas together in one place

nexus is an ongoing personal project and the codebase can be used as a starting point for anyone interested in building their own esp32 based dashboard

working demo video  https://youtu.be/BgzsuMz_hZo
