# NEXUS – ESP32 Smart Dashboard

This is the whole codebase for my **NEXUS ESP32 dashboard**. It has the code, circuit connections and setup information needed to build your own compact smart dashboard using an **ESP32 C3 Super Mini, ST7735 TFT display, MPU6050 motion sensor and rotary encoder**.

The main idea behind this project is to build a small interactive device that combines different useful features into one system. Instead of making separate projects for motion sensing, system information, wifi connectivity and bluetooth controls, i wanted to bring everything together into one dashboard.

The ESP32 C3 handles the main logic, reads the sensors, manages user input and runs the different features. The TFT display provides the interface, while the rotary encoder makes it easy to navigate through the menus.

As the project developed, i also added a web dashboard, bluetooth media controls, a motion controlled space shooter game and an animated screensaver.

## Features

- Compact ESP32 C3 based dashboard
- ST7735 TFT display interface
- Main menu with multiple features
- Rotary encoder based navigation
- MPU6050 accelerometer and gyroscope
- Real time motion sensor readings
- System information screen
- Dedicated encoder testing screen
- Wifi network scanning
- Display of saved wifi networks
- Wifi signal strength indication using RSSI
- Connect to available saved networks
- Wifi connection status display
- Web dashboard
- Bluetooth media controls
- Volume control using bluetooth
- Media play and pause controls
- Motion controlled space shooter game
- Gyroscope calibration for motion based controls
- Animated NEXUS screensaver
- Automatic screensaver after inactivity
- Wake the dashboard using the rotary encoder
- Multiple features integrated into one device
- Compact and easy to expand design

## BOM – Bill of Materials

| Component | Quantity | Purpose | Purchase Link |
|-----------|:--------:|---------|:-------------:|
| ESP32 C3 Super Mini | 1 | Main controller and processing | [Buy Here](https://www.google.com/search?q=ESP32+C3+Super+Mini) |
| ST7735 / ST7735S TFT Display | 1 | Dashboard interface and graphics | [Buy Here](https://www.google.com/search?q=ST7735+TFT+display) |
| MPU6050 Module | 1 | Accelerometer and gyroscope for motion sensing | [Buy Here](https://www.google.com/search?q=MPU6050+module) |
| Rotary Encoder Module | 1 | Menu navigation and user input | [Buy Here](https://www.google.com/search?q=rotary+encoder+module) |
| Jumper Wires | As required | Electrical connections | - |
| Enclosure | As required | Protects the electronics and improves the final design | Self Designed (3D printed) |

The exact components can vary depending on the modules you have available. The display and encoder connections should be checked against your particular hardware before powering everything on.

## Circuit Diagram

I have used the following pin connections for the main NEXUS hardware. You can use them as a reference while connecting your own components.

### Main Connections

**ST7735 TFT Display → ESP32 C3**

| TFT Display | ESP32 C3 |
|-------------|----------|
| RST | GPIO 3 |
| CS | GPIO 7 |
| DC | GPIO 2 |
| MOSI | GPIO 6 |
| SCLK | GPIO 4 |
| VCC | Suitable supply according to the display module |
| GND | GND |

**MPU6050 → ESP32 C3**

| MPU6050 | ESP32 C3 |
|---------|----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 1 |
| SCL | GPIO 5 |

The MPU6050 communicates with the ESP32 using I2C. Its usual I2C address in this setup is `0x68`.

**Rotary Encoder → ESP32 C3**

| Rotary Encoder | ESP32 C3 |
|----------------|----------|
| CLK | GPIO 0 |
| DT | GPIO 10 |
| SW | GPIO 20 |
| GND | GND |
| + | Suitable supply according to the module |

The encoder is used to move through the menus, select options and interact with the different dashboard features.

The exact power connections depend on the modules being used. Check the voltage requirements of the TFT display and encoder before connecting them.

All the components should share a common ground.

## How to Build This...

**step 1:** First get all the components mentioned in the BOM. The main parts required are the ESP32 C3 Super Mini, ST7735 TFT display, MPU6050 and rotary encoder.

**step 2:** Connect the TFT display to the ESP32 using the pin connections mentioned above. Make sure the display pins match the configuration in the code.

**step 3:** Connect the MPU6050 to the ESP32. Connect SDA to GPIO 1 and SCL to GPIO 5. Connect VCC to 3.3V and GND to GND.

**step 4:** Connect the rotary encoder to GPIO 0, GPIO 10 and GPIO 20. These pins are used for rotation detection and button input.

**step 5:** Connect the ESP32 to your computer using a USB cable. Make sure the board is detected properly before uploading the code.

**step 6:** Open Arduino IDE and install the required libraries. The project uses the Adafruit ST7735 display library for the TFT interface. Install any additional libraries required by the particular source files you are using.

**step 7:** Select the correct ESP32 C3 board and port in Arduino IDE. Upload the code and open the serial monitor if you need to check startup messages or debug the device.

**step 8:** Once the device starts, check whether the display is working properly. Test the rotary encoder and make sure the menu changes when you rotate or press it.

**step 9:** Open the motion sensor screen and check whether the MPU6050 readings change when you move the device. If the readings remain fixed, check the I2C wiring and sensor address.

**step 10:** After the basic hardware is working, test the wifi manager, web dashboard and bluetooth media controls. Some features require additional setup on the connected computer or network.

**step 11:** Finally, test the motion controlled game and screensaver. Once everything works properly, you can assemble the components into a compact enclosure.

## Main Menu

The main menu is the starting point of the NEXUS dashboard.

I wanted the interface to be simple enough to use without needing a keyboard or mouse. The rotary encoder provides the main input, so you can move through the available options and select the feature you want.

The main menu includes:

- Motion Sensor
- System Info
- Encoder Test
- Wifi Connect

Each option opens a separate screen with its own functionality.

The menu structure also makes it easier to add new features without changing the entire interface.

## Motion Sensor

The motion sensor screen uses the MPU6050 to read movement data from the device.

The MPU6050 contains an accelerometer and a gyroscope. The accelerometer measures acceleration along different axes, while the gyroscope measures angular velocity.

These readings can be used to understand how the device is moving or rotating.

The motion sensor is also useful for the motion controlled space shooter game, where the movement of the device is used as an input.

The main things to check while testing the sensor are:

- Whether the sensor is detected correctly
- Whether the readings change when the device moves
- Whether the readings remain stable when the device is still
- Whether the sensor orientation matches the expected controls

The sensor can also be used as a starting point for future projects involving gesture control, orientation tracking and motion based interaction.

## System Information

The system information screen provides a place to display useful information about the device.

Instead of connecting the ESP32 to a computer every time i want to check something, the idea is to make important system information available directly on the TFT display.

This screen can also be expanded later to include additional information such as wifi status, uptime, memory usage and other available system statistics.

## Encoder Test

The encoder test screen is used to check whether the rotary encoder is working correctly.

The rotary encoder detects rotation using its CLK and DT pins, while the SW pin is used for button input.

This screen is useful during development because it helps identify problems with the encoder before using it to navigate through the entire dashboard.

If the direction feels reversed, the encoder logic can be adjusted in the code.

## Wifi Manager

The wifi manager allows the device to scan for wireless networks and connect to saved networks.

Instead of showing every available network without any filtering, the interface checks which saved networks are currently visible.

The available information includes the network name and signal strength.

The wifi manager includes:

- Scanning for available wifi networks
- Finding saved networks that are currently available
- Displaying signal strength using RSSI
- Selecting a network to connect to
- Showing the connection status
- Returning to the previous menu
- Rescanning for networks

The connection process also handles different states such as connecting, connected, failed and cancelled.

During testing, i found that the network environment can make a difference. A personal hotspot worked, while the campus wifi did not connect successfully in my testing.

This showed that the problem is not always in the ESP32 code. Network restrictions, authentication requirements and access point configuration can also affect connectivity.

## Web Dashboard

The web dashboard is another part of NEXUS that extends the project beyond the small TFT display.

The idea is to make the device accessible through a browser, providing another way to interact with or view information from the system.

The ESP32 networking functionality provides the foundation for connecting the device to a web based interface.

When testing this feature, make sure the ESP32 and the device opening the dashboard are connected to a network that allows them to communicate with each other.

The exact dashboard address and available controls depend on the web server implementation in the source code.

This feature also provides a foundation for adding more controls, system information and interactive elements in the future.

## Bluetooth Media Controls

I also added bluetooth media controls to NEXUS so that it can interact with a computer without needing a physical keyboard.

The bluetooth device uses the name `NEXUS` and supports media related controls through Bluetooth HID.

The main idea is to control media playback and volume using the dashboard.

The feature includes:

- Bluetooth HID functionality
- Media play and pause controls
- Volume control
- Wireless interaction with a compatible computer

During setup, make sure bluetooth is enabled on the computer and pair with the NEXUS device when it appears.

Bluetooth behavior can vary depending on the operating system and the way the computer handles media keys.

This feature makes the project more useful as a desk gadget instead of being only a sensor dashboard.

## Motion Controlled Space Shooter

One of the more interesting features of NEXUS is the motion controlled space shooter game.

Instead of controlling the game using regular keyboard buttons, the idea is to use the movement of the device as the input.

The MPU6050 provides the motion data, which can be used to control movement within the game.

The game also includes different states such as calibration, shielding and game over.

Calibration is important because the sensor should establish a suitable reference position before the controls are used.

The main things to check while testing the game are:

- Whether the sensor readings respond correctly to movement
- Whether the controls feel natural
- Whether the game responds consistently
- Whether calibration establishes the correct reference position
- Whether the game over and other states work correctly

This feature was a fun way to use the motion sensor for something interactive instead of displaying sensor readings alone.

## Idle Screensaver

I also added an animated screensaver to make the dashboard feel more complete.

When the device remains inactive for around 30 seconds, the screensaver can start automatically.

The screensaver includes an animated star field and a cyan and blue NEXUS logo.

The rotary encoder can be used to wake the dashboard and return to the normal interface.

The screensaver is designed to avoid interrupting activities where the device is still being used, such as gameplay, calibration or an active wifi connection process.

This small addition makes the interface feel more like a finished device rather than a collection of separate test screens.

## Code

The main project uses the following components and concepts:

- Arduino framework for the ESP32 C3
- TFT display library for the ST7735 screen
- I2C communication using `Wire.h`
- MPU6050 accelerometer and gyroscope
- Rotary encoder input handling
- Wifi scanning and connection management
- Web based dashboard functionality
- Bluetooth HID media controls
- Motion based game controls
- Animated screensaver

The pin configuration used by the project is:

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

These values should match the actual wiring of the hardware.

If you change the display, encoder or sensor connections, update the corresponding pin definitions in the code.

The libraries and configuration options required by individual features should be checked in their respective source files.

## Known Issues

1. Wifi connectivity can depend on the network configuration and authentication method.
2. Some campus networks may restrict connections from embedded devices.
3. The TFT display may require different initialization settings depending on the exact ST7735 module.
4. Incorrect display pin connections can cause a blank screen or incorrect output.
5. The rotary encoder may require adjustment if the direction or button input behaves incorrectly.
6. MPU6050 readings can fluctuate because of sensor noise and physical movement.
7. Sensor orientation affects how motion data should be interpreted.
8. Bluetooth pairing and media controls can behave differently across operating systems.
9. The web dashboard requires a suitable network connection between the ESP32 and the connected device.
10. The screensaver timeout and wake behavior should be tested alongside the other dashboard features.
11. Combining multiple features can introduce timing or resource conflicts.
12. Additional features may require changes to the menu structure and input handling.

## CAD Models

The project can be placed inside a custom enclosure to make it easier to carry and use on a desk.

The enclosure should provide enough space for the ESP32 C3, TFT display, MPU6050 and rotary encoder.

The display should be positioned so that it is easy to read, and the encoder should remain accessible for navigation.

The exact enclosure dimensions depend on the display module and the way the components are mounted.

A custom 3D printed enclosure would also make it easier to protect the electronics and give the device a more finished appearance.

## Working Principle

The basic idea behind NEXUS is to use the ESP32 C3 as the main controller and combine multiple features into one compact system.

The display provides the interface, the rotary encoder handles user input, and the MPU6050 supplies motion data.

The wifi and bluetooth functionality provides wireless features, while the web dashboard offers another way to interact with the device.

The overall structure looks something like this:

```text
              ESP32 C3
                  |
       +----------+----------+
       |          |          |
    Display     Encoder    MPU6050
       |          |          |
       +----------+----------+
                  |
             Main Menu
                  |
       +----------+----------+
       |          |          |
      Wifi     Bluetooth    Motion
       |          |          |
   Web Dashboard Media      Game
                  Controls
                  |
             Screensaver
```

The ESP32 continuously handles the logic required by the active feature and updates the interface when needed.

When the user rotates or presses the encoder, the input is processed by the menu system. Selecting a feature opens the corresponding screen or starts the relevant functionality.

The motion sensor provides data for motion related features, while wifi and bluetooth enable wireless interaction.

All these parts work together to turn a small ESP32 board into a multi feature dashboard.

## Final Result

The whole idea behind NEXUS was to build something more useful than a basic sensor display.

I started with a small ESP32 C3, a TFT display, an MPU6050 and a rotary encoder. From there, i gradually brought different features together into one device.

The project now combines menu navigation, motion sensing, wifi connectivity, a web dashboard, bluetooth media controls, a motion controlled game and an animated screensaver.

It was also a good way to learn more about embedded programming, sensor integration, display interfaces, wireless communication and combining multiple features without making the user interface difficult to navigate.

There is still room to improve the design, add more functionality and build a proper enclosure, but the main goal was to create a compact device that brings several useful ideas together in one place.

NEXUS is an ongoing personal project, and the codebase can be used as a starting point for anyone interested in building their own ESP32 based dashboard.

---

**Project:** NEXUS ESP32 Smart Dashboard

**Controller:** ESP32 C3 Super Mini

**Display:** ST7735 / ST7735S TFT

**Motion Sensor:** MPU6050

**Input:** Rotary Encoder

**Framework:** Arduino

**Main Areas:** Embedded Systems, Sensor Integration, Wifi, Bluetooth HID and UI Development
