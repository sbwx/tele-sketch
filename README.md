# tele-sketch: Real-time Wireless Sketching with ESP-IDF

**`tele-sketch`** is an ESP-IDF project designed to enable remote sketching or drawing functionality. It uses Wi-Fi/Bluetooth to send sensor data (like gyroscope/accelerometer input or touch coordinates) from an ESP32 device to a host application, where the data is translated into line art or graphics.

---

## Features

* **Wireless Communication:** Uses **Wi-Fi** for data transmission.

---

## Hardware and Software Requirements

### Hardware

* **ESP32/ESP32-S3 Microcontroller:** ESP32-S3-N16R8

### Software

* **ESP-IDF v5.3.1:**
* **CMake**
* **Python:**

---

## Getting Started

### 1. Project Setup (ESP-IDF)

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/sbwx/tele-sketch.git](https://github.com/sbwx/tele-sketch.git)
    cd tele-sketch
    ```
2.  **Configure the project:**
    ```bash
    idf.py menuconfig
    ```
    * Set your **Serial Flasher config** (e.g., Baud rate, Port).
    * Go to **Component config -> Wi-Fi** (or **Bluetooth**) and configure your connectivity settings.
    * **CRITICAL:** Define your Wi-Fi credentials (SSID and Password) in the appropriate configuration file.

3.  **Build, Flash, and Monitor:**
    ```bash
    idf.py flash monitor
    ```

### 2. Host Application Setup

    1.  Install necessary Python dependencies: `pip install -r requirements.txt`

---

## Project Structure

```text
tele-sketch/
├── components/
│   └── LovyanGFX/   # Your graphics library (tracked via git or submodule)
├── main/
│   ├── main.cpp      # Main application logic (sensor reading, Wi-Fi setup)
│   └── main.h
├── build/            # IGNORED by git - build output
├── CMakeLists.txt
├── .gitignore        # Defines ignored files (e.g., build/, sdkconfig)
└── README.md         # This file
