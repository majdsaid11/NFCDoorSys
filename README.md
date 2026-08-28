# 📡 ESP32 NFC Web Station (PN532)

An interactive, responsive web-based NFC management console powered by the **ESP32** and the **PN532 NFC module** via I2C interface. This project allows you to scan, read, and write to MIFARE Classic RFID cards directly from any modern web browser.

---

## ✨ Features

- **🌐 Live Web Console**: Beautiful modern dark UI to manage your NFC station remotely.
- **🔍 Card Identification**: Instant live scanning to fetch UID and autodetect the card type (MIFARE Classic, Ultralight, NTAG).
- **📖 Block Reading**: Full support for scanning specific sectors/blocks and decoding hex values into readable ASCII text.
- **✍️ Block Writing**: Custom payload injection into blocks using configurable authorization Keys (Default Key A: `FFFFFFFFFFFF`).
- **🛡️ Embedded Safeguards**: Client-side warnings to prevent accidental overwrites on sensitive manufacturer/trailer blocks (`0, 3, 7, 11, etc.`).

---

## 🛠️ Hardware Requirements

| Component | Description |
| :--- | :--- |
| **ESP32 Board** | NodeMCU / ESP32 Development board |
| **PN532 Module** | Configured in **I2C Mode** (Ensure hardware dip-switches are set correctly) |
| **NFC Tags** | MIFARE Classic 1K / 4K / Ultralight tags or cards |

### 📌 Wiring Configuration (I2C)

| PN532 Pin | ESP32 Pin | Description |
| :---: | :---: | :--- |
| **VCC** | `3V3` | Power Input |
| **GND** | `GND` | Ground Reference |
| **SDA** | `GPIO 21` | I2C Data Line |
| **SCL** | `GPIO 22` | I2C Clock Line |

*⚠️ **Note**: Make sure the tiny dip-switches on your PN532 board are set to **I2C mode** (usually `Channel 1: ON`, `Channel 2: OFF` depending on the board manufacturer).*

---

## 💻 Library Dependencies

Ensure you have the following libraries installed via the Arduino Library Manager before compiling:
- **Adafruit PN532** by Adafruit
- **WiFi** (Built-in ESP32 core library)
- **WebServer** (Built-in ESP32 core library)
- **Wire** (Built-in ESP32 core library)

---

## 🚀 Setup & Installation

1. Open the project inside your **Arduino IDE** or **VS Code (PlatformIO)**.
2. Update your local network credentials inside the `NfcD.ino` file:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Upload the code to your ESP32 board.
4. Open the **Serial Monitor** at a baud rate of `115200`.
5. Once connected, your ESP32 will print out its local IP address (e.g., `115200 IP Address: 192.168.1.50`).
6. Open your favorite browser and visit that IP address to launch the dashboard!

---

## 📸 Dashboard Interface Overview

### 1. Card Scanner
Clicking **SCAN CARD** invokes the PN532 passive target loop. It displays the raw HEX UID and runs logic to guess if the chip is standard classic or NTAG.

### 2. Hex Key Authorization
Provide custom authentication keys to open blocks before modification. Default manufacturing states use standard standard `FF` fields.

### 3. Read/Write Subsystem
Reads 16-byte blocks or packs strings to write into raw sectors. Avoid modifying sensitive address arrays to maintain proper tag lifecycles.

---

## 📄 License
This project is open-source and available under the **MIT License**. Feel free to tweak, scale, and build upon it!
