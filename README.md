# tele-sketch: Real-time Wireless Sketching with ESP-IDF

**`tele-sketch`** is an ESP-IDF project designed to enable remote sketching or drawing functionality. It uses Wi-Fi to send ADC readings read from a joystick connected to an ESP32 device to a host application, where the data is translated into line art or graphics.

---

## Features

* **Wireless Communication:** Uses **Wi-Fi** for data transmission.
* **Joystick Controls:** Uses a joystick for an interactive drawing experience.
* **Visual Clarity:** Uses a TFT LCD screen driven by an ILI9486 for great visual clarity streamed at 60FPS.

---

## Hardware and Software Requirements

### Hardware

* **Microcontroller:** ESP32-S3-N16R8
* **Screen:** ILI9486 (4" TFT SPI LCD)

### Software

* **ESP-IDF v5.3.1**
* **CMake**
* **Python**
