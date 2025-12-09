# tele-sketch: Real-time Wireless Sketching with ESP-IDF

**`tele-sketch`** is an ESP-IDF project designed to enable remote sketching or drawing functionality. It uses Wi-Fi/Bluetooth to send sensor data (like gyroscope/accelerometer input or touch coordinates) from an ESP32 device to a host application, where the data is translated into line art or graphics.

---

## Features

* **Wireless Communication:** Uses **Wi-Fi** (or Bluetooth) for data transmission.
* **Real-time Data:** Streams sensor/input data in real-time for immediate visualization.
* **[Specific Sensor] Integration:** Reads input from a **[Specify your sensor, e.g., MPU6050, Capacitive Touch Panel, Joystick]**.
* **[Specific Protocol] Protocol:** Data is packaged and sent via **[Specify your protocol, e.g., MQTT, WebSocket, Custom UDP]**.

---

## Hardware and Software Requirements

### Hardware

* **ESP32/ESP32-S3 Microcontroller:** (Specify your exact board model if known, e.g., ESP32 DevKitC)
* **[Specific Sensor] Module:** (e.g., MPU6050, ST7789 Display)
* **Host Device:** A computer running **Python/Processing/Node.js** to visualize the sketch.

### Software

* **ESP-IDF vX.X:** (Specify your version, e.g., v5.1)
* **CMake**
* **[Host Software Language]:** (e.g., Python, with libraries like `pyserial` or `websockets`)

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

* *(If you have a separate host application, provide steps here:)*
    1.  Install necessary Python dependencies: `pip install -r requirements.txt`
    2.  Run the receiver script: `python host_app/receiver.py`

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
