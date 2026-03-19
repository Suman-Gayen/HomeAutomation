# Smart Home Automation System - ESP32 Documentation

## Overview
This project implements a comprehensive smart home automation system using an **ESP32 microcontroller** with **ESP-RainMaker** IoT platform integration. It enables remote control and monitoring of multiple devices through WiFi connectivity with support for manual switches and sensor data collection.

---

## Table of Contents
1. [Features](#features)
2. [Hardware Components](#hardware-components)
3. [Pin Configuration](#pin-configuration)
4. [Software Components](#software-components)
5. [Code Explanation](#code-explanation)
6. [Function Reference](#function-reference)
7. [Setup Instructions](#setup-instructions)
8. [Usage](#usage)
9. [Troubleshooting](#troubleshooting)

---

## Features

✅ **Remote Control**: Control 4 relay switches via WiFi through ESP-RainMaker app  
✅ **Manual Switches**: Physical switches for local control of each relay  
✅ **Temperature Monitoring**: DHT11 sensor for real-time temperature readings  
✅ **Humidity Monitoring**: DHT11 sensor for real-time humidity readings  
✅ **Light Intensity Sensing**: LDR (Light Dependent Resistor) for ambient light detection  
✅ **WiFi Provisioning**: BLE or SoftAP-based WiFi provisioning  
✅ **Factory Reset**: Long-press button for factory reset or WiFi reset functionality  
✅ **OTA Updates**: Over-the-air firmware update support  
✅ **Scheduling**: Built-in scheduling capabilities via RainMaker  
✅ **Real-time Status Feedback**: WiFi status indication via LED

---

## Hardware Components

### Microcontroller
- **ESP32** (DevKit or equivalent)

### Sensors
- **DHT11**: Temperature and Humidity Sensor (connected to GPIO 18)
- **LDR**: Light Dependent Resistor for light intensity measurement (connected to GPIO 39 / VN pin)

### Actuators
- **4x Relay Modules**: For controlling high-power devices

### Control Elements
- **4x Manual Switches**: For local control of relays
- **1x Reset Button**: For WiFi/Factory reset operations
- **1x WiFi Status LED**: Indicates WiFi connection status

### Power Supply
- 5V power supply for relays and sensors
- USB power for ESP32 development

---

## Pin Configuration

| Function | GPIO Pin | Pin Label | Purpose |
|----------|----------|-----------|---------|
| **Relay 1** | 23 | D23 | Relay control (e.g., Light 1) |
| **Relay 2** | 22 | D22 | Relay control (e.g., Light 2) |
| **Relay 3** | 21 | D21 | Relay control (e.g., Fan) |
| **Relay 4** | 19 | D19 | Relay control (e.g., Water Pump) |
| **Switch 1** | 13 | D13 | Manual control for Relay 1 |
| **Switch 2** | 12 | D12 | Manual control for Relay 2 |
| **Switch 3** | 14 | D14 | Manual control for Relay 3 |
| **Switch 4** | 27 | D27 | Manual control for Relay 4 |
| **WiFi Status LED** | 2 | D2 | WiFi connection indicator |
| **Reset Button** | 0 | D0 | Factory/WiFi reset trigger |
| **DHT Sensor** | 18 | D18 | Temperature & Humidity input |
| **LDR Sensor** | 39 | VN | Light intensity measurement |

---

## Software Components

### Libraries Used

```cpp
#include "RMaker.h"        // ESP-RainMaker library for IoT connectivity
#include "WiFi.h"          // WiFi connectivity
#include "WiFiProv.h"      // WiFi Provisioning (BLE/SoftAP)
#include <DHT.h>           // DHT sensor library
#include <SimpleTimer.h>   // Timer for periodic tasks
```

### Key Constants

```cpp
const char *service_name = "PROV_SmartHome";  // Service name for provisioning
const char *pop = "1234";                      // Proof of Possession
uint32_t espChipId = 5;                        // Unique chip identifier
char nodeName[] = "ESP32_Smarthome";           // RainMaker node name
```

### Device Names

- `Switch1` - First controlled relay
- `Switch2` - Second controlled relay
- `Switch3` - Third controlled relay
- `Switch4` - Fourth controlled relay
- `Temperature` - Temperature sensor data
- `Humidity` - Humidity sensor data
- `LDR` - Light intensity data

---

## Code Explanation

### Global Variables

#### Relay and Switch States
```cpp
bool toggleState_1 to 4;  // Track relay ON/OFF state
bool SwitchState_1 to 4;  // Track switch press state
```

#### Sensor Data
```cpp
float temperature1;  // Current temperature reading
float humidity1;     // Current humidity reading
float ldrVal;        // Current light intensity (0-100%)
```

#### Device Objects
```cpp
Switch my_switch1 to 4;                    // Relay control objects
TemperatureSensor temperature, humidity, ldr;  // Sensor objects
SimpleTimer Timer;                         // Timer for sensor reading
```

---

## Function Reference

### 1. **sysProvEvent(arduino_event_t *sys_event)**
Handles WiFi provisioning events.

**Events Handled:**
- `ARDUINO_EVENT_PROV_START`: Displays QR code for BLE/SoftAP provisioning
- `ARDUINO_EVENT_WIFI_STA_CONNECTED`: Turns on WiFi LED when connected

```cpp
void sysProvEvent(arduino_event_t *sys_event) {
  // Handles provisioning and WiFi connection events
  // Displays QR code for device pairing
  // Controls WiFi status LED
}
```

---

### 2. **write_callback(Device, Param, param_val_t, void, write_ctx_t)**
Handles remote control commands from RainMaker app.

**Functionality:**
- Receives relay toggle commands
- Compares device name to identify which relay
- Updates relay GPIO state (inverted logic: LOW = ON, HIGH = OFF)
- Reports parameter change back to RainMaker

**Example:**
```cpp
// When Switch1 Power is set to true via app:
toggleState_1 = true;
digitalWrite(RelayPin1, LOW);  // Relay activates
param->updateAndReport(val);   // Confirms to app
```

**Relay Logic:**
- `toggleState = true` → `digitalWrite(RelayPin, LOW)` → Relay ON
- `toggleState = false` → `digitalWrite(RelayPin, HIGH)` → Relay OFF

---

### 3. **readSensor()**
Reads sensor data from DHT11 and LDR.

**Operations:**
1. **LDR Reading**: Converts analog reading to 0-100% brightness scale
   ```cpp
   ldrVal = map(analogRead(LDR_PIN), 400, 4200, 0, 100);
   ```
   - Raw ADC: 400-4200 → Mapped: 0-100%

2. **DHT Reading**: Gets temperature and humidity
   ```cpp
   float h = dht.readHumidity();
   float t = dht.readTemperature();
   ```

3. **Error Handling**: Checks for read failures
4. **Serial Output**: Logs sensor values for debugging

---

### 4. **sendSensor()**
Reads sensors and uploads data to RainMaker cloud.

**Process:**
1. Calls `readSensor()` to get fresh data
2. Updates temperature parameter: `temperature.updateAndReportParam("Temperature", temperature1);`
3. Updates humidity parameter: `humidity.updateAndReportParam("Temperature", humidity1);`
4. Updates LDR parameter: `ldr.updateAndReportParam("Temperature", ldrVal);`

**Note:** All three sensors report to "Temperature" param (can be customized)

---

### 5. **manual_control()**
Handles physical switch inputs for local control.

**Logic for Each Switch (Example - Switch 1):**
```cpp
if (digitalRead(SwitchPin1) == LOW && SwitchState_1 == LOW) {
  // Switch pressed (LOW due to PULLUP)
  digitalWrite(RelayPin1, LOW);        // Turn relay ON
  toggleState_1 = 1;                   // Update state
  SwitchState_1 = HIGH;                // Mark switch as pressed
  my_switch1.updateAndReportParam(...); // Update RainMaker
  Serial.println("Switch-1 on");       // Debug output
}

if (digitalRead(SwitchPin1) == HIGH && SwitchState_1 == HIGH) {
  // Switch released
  digitalWrite(RelayPin1, HIGH);       // Turn relay OFF
  toggleState_1 = 0;                   // Update state
  SwitchState_1 = LOW;                 // Mark switch as released
  my_switch1.updateAndReportParam(...); // Update RainMaker
  Serial.println("Switch-1 off");      // Debug output
}
```

**Repeated for all 4 switches**

---

### 6. **setup()**
Initializes all hardware and software components.

**Initialization Steps:**

1. **Serial Communication**
   ```cpp
   Serial.begin(115200);  // Debug serial output
   ```

2. **GPIO Configuration**
   ```cpp
   pinMode(RelayPin1-4, OUTPUT);           // Relay pins as outputs
   pinMode(SwitchPin1-4, INPUT_PULLUP);    // Switch pins with pullup
   pinMode(gpio_reset, INPUT);             // Reset button input
   pinMode(wifiLed, OUTPUT);               // WiFi LED output
   ```

3. **Initial State**
   ```cpp
   digitalWrite(RelayPin1-4, !toggleState);  // All relays OFF
   digitalWrite(wifiLed, LOW);              // LED OFF
   ```

4. **DHT Sensor Initialization**
   ```cpp
   dht.begin();  // Initialize DHT11
   ```

5. **RainMaker Node Creation**
   ```cpp
   Node my_node = RMaker.initNode(nodeName);
   ```

6. **Device Registration**
   ```cpp
   my_switch1.addCb(write_callback);  // Register callback for remote control
   my_node.addDevice(my_switch1-4);   // Add all switches
   my_node.addDevice(temperature);    // Add sensors
   my_node.addDevice(humidity);
   my_node.addDevice(ldr);
   ```

7. **Timer Setup**
   ```cpp
   Timer.setInterval(2000);  // 2-second interval for sensor reads
   ```

8. **RainMaker Configuration**
   ```cpp
   RMaker.enableOTA(OTA_USING_PARAMS);  // Over-the-air updates
   RMaker.enableTZService();            // Timezone service
   RMaker.enableSchedule();             // Scheduling feature
   ```

9. **WiFi Provisioning**
   ```cpp
   #if CONFIG_IDF_TARGET_ESP32
     WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, ...);  // BLE method
   #else
     WiFiProv.beginProvision(WIFI_PROV_SCHEME_SOFTAP, ...); // SoftAP method
   #endif
   ```

10. **Device State Reset**
    ```cpp
    my_switch1-4.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    ```

---

### 7. **loop()**
Main program loop - runs repeatedly.

**Operations:**

1. **Reset Button Handling**
   ```cpp
   if (digitalRead(gpio_reset) == LOW) {
     // Debounce delay
     delay(100);
     int startTime = millis();
     
     while (digitalRead(gpio_reset) == LOW) delay(50);
     int endTime = millis();
     
     // Duration determines action:
     if ((endTime - startTime) > 10000) {
       RMakerFactoryReset(2);  // >10 seconds: Full factory reset
     } else if ((endTime - startTime) > 3000) {
       RMakerWiFiReset(2);     // 3-10 seconds: WiFi reset only
     }
   }
   ```

2. **WiFi Status Check**
   ```cpp
   if (WiFi.status() != WL_CONNECTED) {
     digitalWrite(wifiLed, false);  // LED OFF - No WiFi
   } else {
     digitalWrite(wifiLed, true);   // LED ON - Connected
     
     // Send sensor data every 2 seconds when connected
     if (Timer.isReady()) {
       sendSensor();
       Timer.reset();
     }
   }
   ```

3. **Manual Switch Processing**
   ```cpp
   manual_control();  // Check physical switches
   ```

4. **Loop Delay**
   ```cpp
   delay(100);  // 100ms delay to prevent overwhelming CPU
   ```

---

## Setup Instructions

### Hardware Setup

1. **Connect ESP32 to Components:**
   - Connect 4 relays to GPIO pins (23, 22, 21, 19)
   - Connect 4 manual switches to GPIO pins (13, 12, 14, 27) with pullup resistors
   - Connect DHT11 sensor to GPIO 18
   - Connect LDR sensor to GPIO 39
   - Connect WiFi status LED to GPIO 2
   - Connect reset button to GPIO 0
   - Provide proper power supply (5V for relays, USB for ESP32)

2. **Wiring Diagram:**
   ```
   Relay Modules (Active Low):
   - Relay 1 (GPIO 23) - Light 1
   - Relay 2 (GPIO 22) - Light 2
   - Relay 3 (GPIO 21) - Fan
   - Relay 4 (GPIO 19) - Water Pump
   
   Manual Switches (with 10K pullup to 3.3V):
   - SW1 (GPIO 13) - Controls Relay 1
   - SW2 (GPIO 12) - Controls Relay 2
   - SW3 (GPIO 14) - Controls Relay 3
   - SW4 (GPIO 27) - Controls Relay 4
   
   Sensors:
   - DHT11 (GPIO 18) - Temperature & Humidity
   - LDR (GPIO 39/VN) - Light intensity
   - Reset Button (GPIO 0) - Factory reset
   - WiFi LED (GPIO 2) - Status indicator
   ```

### Software Setup

1. **Install Arduino IDE** and ESP32 board support

2. **Install Required Libraries:**
   - ESP-RainMaker library
   - DHT library (by Adafruit)
   - SimpleTimer library

3. **Add RainMaker to Arduino:**
   - Install ESP32 board package in Arduino IDE
   - Install "Arduino-esp32-rainmaker" library

4. **Upload Code:**
   - Open `home_esp.ino` in Arduino IDE
   - Select **Board:** ESP32 Dev Module
   - Select **Port:** COM port of ESP32
   - Click **Upload**

5. **Initial Provisioning:**
   - On first boot, device enters provisioning mode
   - Use EspressIf RainMaker app to scan QR code
   - Provide WiFi credentials
   - Device connects to cloud

---

## Usage

### Remote Control via RainMaker App

1. **Download** EspressIf RainMaker app (iOS/Android)
2. **Scan** QR code displayed on Serial monitor during provisioning
3. **Add Device** to your account
4. **Control Switches:** Toggle each relay on/off remotely
5. **Monitor Sensors:** View real-time temperature, humidity, and light levels
6. **Set Schedules:** Create automation rules (e.g., turn on light at 6 PM)

### Local Control via Physical Switches

1. **Press** any manual switch to toggle corresponding relay
2. **Status updates** reflect immediately in RainMaker app
3. **No WiFi needed** for local control - works offline

### Factory Reset

**Procedure:**
1. Press and hold **GPIO 0 button** for more than 10 seconds
2. Device will factory reset and restart in provisioning mode
3. Re-provision with WiFi credentials

### WiFi Reset Only

**Procedure:**
1. Press and hold **GPIO 0 button** for 3-10 seconds
2. WiFi credentials cleared, restart provisioning
3. All other settings retained

---

## Troubleshooting

### Issue: Device Not Provisioning

**Solution:**
- Check if BLE or SoftAP is enabled based on board type
- Verify GPIO 0 is not stuck LOW
- Check Serial monitor for error messages
- Try factory reset (hold GPIO 0 for 10+ seconds)

### Issue: Sensors Not Reading

**Solution:**
- Verify DHT11 connections to GPIO 18
- Check LDR connections to GPIO 39
- Test with serial print statements
- Ensure DHT library is installed

### Issue: Relays Not Activating

**Solution:**
- Check relay GPIO connections (23, 22, 21, 19)
- Verify relay modules have proper power supply
- Test with digitalWrite() commands in Serial monitor
- Check for inverted logic (LOW = ON, HIGH = OFF)

### Issue: WiFi LED Not Working

**Solution:**
- Verify LED connected to GPIO 2 with current-limiting resistor
- Check if WiFi connection is successful (Serial monitor)
- Test GPIO 2 with test code

### Issue: Manual Switches Not Working

**Solution:**
- Verify switch connections to GPIO pins (13, 12, 14, 27)
- Check for 10K pullup resistors
- Test with digitalRead() in Serial monitor
- Verify debounce logic in `manual_control()` function

### Issue: Unstable WiFi Connection

**Solution:**
- Check WiFi signal strength
- Verify antenna placement
- Try changing WiFi channel
- Update ESP32 board package
- Check power supply stability

---

## Performance Specifications

| Parameter | Value |
|-----------|-------|
| Sensor Read Interval | 2 seconds |
| Loop Delay | 100 ms |
| Baud Rate (Serial) | 115200 bps |
| LDR Mapping Range | 400-4200 ADC units |
| DHT Sensor Type | DHT11 |
| Max Relays | 4 |
| Max Manual Switches | 4 |
| Reset Detection | 3-10 seconds (WiFi), 10+ seconds (Factory) |

---

## Future Enhancements

- Add MQTT support for third-party integrations
- Implement voice control (Alexa/Google Home)
- Add motion sensor for automation
- Implement power consumption monitoring
- Add mobile app with advanced analytics
- Support for more relay outputs
- Data logging to SD card or cloud

---

## License

This project is open-source and available for educational and personal use.

---

## Support & Contributions

For issues, questions, or contributions, please refer to the repository issue tracker.

---

**Last Updated:** 2026-03-19 15:34:51
**Author:** Suman-Gayen
**Repository:** Suman-Gayen/HomeAutomation
