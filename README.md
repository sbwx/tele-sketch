# tele-sketch: Real-time Wireless Sketching with ESP-IDF

**`tele-sketch`** is an ESP-IDF project designed to enable remote sketching or drawing functionality. It uses Wi-Fi/Bluetooth to send sensor data (like gyroscope/accelerometer input or touch coordinates) from an ESP32 device to a host application, where the data is translated into line art or graphics.

---

## Features

* **Wireless Communication:** Uses **Wi-Fi** for data transmission.
* **Joystick Controls:** Uses a **joystick** for interactive drawing experience.
* **4" TFT LCD Screen:** Uses a TFT LCD screen driven by an ILI9486 for great visual clarity.

---

## Hardware and Software Requirements

### Hardware

* **ESP32/ESP32-S3 Microcontroller:** ESP32-S3-N16R8

### Software

* **ESP-IDF v5.3.1**
* **CMake**
* **Python**
