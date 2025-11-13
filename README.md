# ESP32 4-Relay Home Automation via RainMaker

## Table of Contents
- [Project Overview](#project-overview)
- [Features](#features)
- [Hardware Preparation](#hardware-preparation)
- [Step 1: Setting Up Arduino IDE & ESP32 Board](#step-1-setting-up-arduino-ide--esp32-board)
- [Step 2: Installing Necessary Libraries & Drivers](#step-2-installing-necessary-libraries--drivers)
- [Step 3: Choosing the Board & Partition Scheme](#step-3-choosing-the-board--partition-scheme)
- [Step 4: Uploading the Code](#step-4-uploading-the-code)
- [Step 5: RainMaker Cloud Setup & Device Integration](#step-5-rainmaker-cloud-setup--device-integration)
- [Technical Details & Code Explanation](#technical-details--code-explanation)
- [Reset/Boot Button Operation](#resetboot-button-operation)
- [Common Issues & Solutions](#common-issues--solutions)
- [Project Photos](#project-photo)
- [Credits & Further Learning](#credits--further-learning)

## Project Overview
This project enables **smart control of 4 relays with ESP32**, using AC appliances locally (manual switches), from the RainMaker app, and voice commands via Google Home and Alexa. The system features state persistence (EEPROM), live feedback, setup by BLE, and robust reset options.

## Features
- **4 relay channels**: control via mobile app, voice assistant, and physical switches
- **Manual On/Off switches** for instant local control
- **Google Home / Alexa integration** (via RainMaker cloud)
- **EEPROM state restore:** Remembers relay states after power loss
- **Real-time feedback:** App, voice, and manual sync
- **Over-the-air updates** (OTA enabled by default)
- **Factory & Wi-Fi reset via BOOT button**
- **Highly commented source code for easy learning**

## Hardware Preparation
- **ESP32 Dev Module** (recommended, see below for board selection issues)
- **4-channel relay board**
- **Momentary push buttons or toggle switches for each relay**
- **Micro-USB data cable** (not just charging; must transfer data)
- **Optional:** breadboard/jumper wires

## Step 1: Setting Up Arduino IDE & ESP32 Board

**1. Download and install the latest Arduino IDE**

**2. Add ESP32 Board Support**
  - Go to `File` > `Preferences` > `Additional Board Manager URLs`
  - Paste: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` 

**3. Open `Tools` > `Board` > `Boards Manager` and search for `ESP32` by Espressif Systems, then install it.**

**4. Select the right board:**
  - Choose **ESP32 Dev Module** in `Tools` > `Board`. This board supports partition schemes (critical for large code sketches, see Step 3).
  - Boards like `DOIT ESP32 DEVKIT V1` often *lack* partition scheme options and can trigger "Sketch too big" errors. If you face this, switch to `ESP32 Dev Module` or other board with partition scheme selector ([details in next section]).

## Step 2: Installing Necessary Libraries & Drivers

**Required Libraries:**
- `AceButton` (install via Arduino Library Manager or from [GitHub](https://github.com/bxparks/AceButton))
- `ESP RainMaker` (pre-installed as part of ESP32 board support)

**USB-To-Serial Bridge Drivers:**
- If the COM port does **not** show up in Arduino IDE, install [CP210x](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) driver 
- Use a proper **micro-USB data cable**; charging-only cables will not work for upload


## Step 3: Choosing the Board & Partition Scheme

The ESP RainMaker library and multi-relay code **require extra firmware space**. To avoid "Sketch too big; text section exceeds available space" errors, follow these steps:

- In Arduino IDE, after choosing `ESP32 Dev Module`, go to **`Tools` > `Partition Scheme`**.
- Select:
  - **RainMaker 4MB No OTA** (if you need maximal sketch space and do not use OTA) (It worked for me)
- Tip: Boards without this option (like `DOIT ESP32 DEVKIT V1`) can trigger upload errors. *Always select a board with partitioning support.*

## Step 4: Uploading the Code

- After compiling, open **Serial Monitor** and set the baud rate to `115200` (matching the code).
- During upload, if you get a connection error, hold down the **BOOT** button on your ESP32 and release when the writing starts
- Commented code aids debugging—no line left unexplained.

## Step 5: RainMaker Cloud Setup & Device Integration

1. **Install the ESP RainMaker app** on your phone.
2. **Provision the device:**
   - Use BLE onboarding—app will connect to ESP32.
   - If your WiFi network doesn't appear in the app, use "Join other network" and enter SSID/password manually (RainMaker only supports 2.4 GHz WiFi).
3. **Device appears in app after provisioning.**
4. **Integration with Google Home / Alexa:**

#### Integrating with Amazon Alexa
1. **Install and sign in to the Alexa app** on your phone (Android/iOS).
2. **Open the ESP RainMaker app** and ensure your devices are visible and working.
3. **Go to app settings in ESP RainMaker.**
   - Tap the **"Settings"** gear icon.
   - Locate and select "Voice Services."
   - Tap **"Amazon Alexa"** and then **"Link with Amazon Alexa."**
4. **Sign in with your ESP RainMaker credentials** when prompted.
5. **Follow the on-screen instructions** to finish linking. The Alexa app will now automatically discover your ESP RainMaker devices.
   - Your "Switch1", "Switch2", etc. will appear as controllable devices.
6. **Test voice commands,** e.g., "Alexa, turn on Switch1."

Tips:
- Make sure device names in your code ("Switch1" ...) are easy to pronounce.
- Linking must use the **same account/email** on both RainMaker and Alexa for device discovery to succeed.

#### Integrating with Google Home
1. **Install and sign in to the Google Home app** on your phone.
2. In Google Home, **create a new Home** if you haven't already.
3. Tap **"+" (Add)** in the top left corner and choose **Set up device → Works with Google.**
4. In the search bar, **type "RainMaker"** and select **ESP RainMaker**.
5. **Sign in with your ESP RainMaker account.**
6. **Grant permissions** and complete the linking process as instructed. Your devices will be imported - you should now see them in the Google Home dashboard.
7. **Test with "Hey Google, turn on Switch1"** or similar commands.

## Technical Details & Code Explanation

- **Every important section in the code is commented**, explaining:
   - ACEButton handling for manual switch logic
   - EEPROM integration for state restoration
   - Active LOW relay signaling
   - RainMaker app/voice control callback
   - WiFi/BLE provisioning and troubleshooting
- See source code for full walkthrough.

## Reset/Boot Button Operation
- **Boot/Reset Button <3s:** No effect
- **Press for 3s - 10s:** Triggers **WiFi reset**; ESP32 loses WiFi credentials and needs reprovisioning (app pairing stays)
- **Press for >10s:** Triggers **Factory reset**; complete wipe of all configs, ESP32 needs full re-setup through RainMaker app

## Common Issues & Solutions
- **Partition scheme errors:** If "Sketch too big," switch board type or partition scheme as described in [Step 3](#step-3-choosing-the-board--partition-scheme).
- **Board detection/COM port:** Use CP210x driver and proper cable ([setup instructions]); if upload fails, check RESET/BOOT button procedure.[5][1]
- **RainMaker WiFi onboarding issues:** App can't find SSID? Use manual network entry and verify you're on a 2.4 GHz WiFi network.
- **WiFi/BLE provisioning bug:** If you see errors for undefined `WIFI_PROV_SCHEME_*` flags, update to `NETWORK_PROV_SCHEME_*` family as used in code. This is the issue I solved in the original code from the video.

## Project Photo
<img src="https://github.com/user-attachments/assets/d4ec2732-de57-46ed-a108-b0220af60833" width="400" height="400" />

## Credits & Further Learning
- Code based on 
  - Video tutorial by Tech StudyCell: [ESP RainMaker + Manual Switch/EEPROM | YouTube](https://www.youtube.com/watch?v=AitCKcyjHuQ)
- If you prefer a guided walkthrough, watch the above video.
- For ESP32 and Arduino IDE setup step-by-step, see:
  - [RandomNerdTutorials Guide](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/)
- For technical help and bug fixes, see code comments and Common Issues and Solutions section

