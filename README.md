#  ESP32 Smart Environment

An IoT-based smart environment monitoring and control system built with **ESP32 and ESP-IDF**.

The system collects environmental data from sensors, provides a web-based dashboard hosted directly on the ESP32, and allows users to control external devices through GPIO and PWM.

##  Features

* Temperature monitoring using **DHT22**
* Humidity monitoring using **DHT22**
* Ambient light monitoring using **BH1750**
* Soil moisture monitoring interface
* Real-time web dashboard
* Remote GPIO control
* Relay control for external devices
* PWM output control
* Wi-Fi Station mode
* Embedded HTML dashboard served directly from ESP32
* Modular ESP-IDF component architecture
* Responsive web interface for desktop and mobile browsers

---

## System Architecture

```text
                  ┌──────────────────┐
                  │      DHT22       │
                  │ Temp / Humidity  │
                  └────────┬─────────┘
                           │
                  ┌────────▼─────────┐
                  │      BH1750      │
                  │   Light (Lux)    │
                  └────────┬─────────┘
                           │
                           ▼
                  ┌──────────────────┐
                  │      ESP32       │
                  │    ESP-IDF       │
                  │                  │
                  │ ┌──────────────┐ │
                  │ │ Sensor Layer │ │
                  │ └──────────────┘ │
                  │                  │
                  │ ┌──────────────┐ │
                  │ │ HTTP Server  │ │
                  │ └──────┬───────┘ │
                  │        │         │
                  │ ┌──────▼───────┐ │
                  │ │ GPIO / PWM   │ │
                  │ └──────────────┘ │
                  └────────┬─────────┘
                           │
                     Wi-Fi Network
                           │
                           ▼
                  ┌──────────────────┐
                  │  Web Dashboard   │
                  │                  │
                  │ Monitoring       │
                  │ Control          │
                  │ Charts           │
                  └──────────────────┘
```

---

## Hardware

| Component            | Function                     |
| -------------------- | ---------------------------- |
| ESP32                | Main microcontroller         |
| DHT22                | Temperature and humidity     |
| BH1750               | Ambient light measurement    |
| Relay 1              | Water pump / external load   |
| Relay 2              | Growth light / external load |
| PWM output           | Motor/fan speed control      |
| Soil moisture sensor | Soil moisture monitoring     |

> Pin assignments are configured in the project components/configuration files.

---

## Software Stack

* **MCU:** ESP32
* **Framework:** ESP-IDF
* **Language:** C
* **Build system:** CMake
* **RTOS:** FreeRTOS
* **Web server:** ESP-IDF HTTP Server
* **JSON:** cJSON
* **Frontend:** HTML / JavaScript
The project uses ESP-IDF's component-based architecture. The main component requires `web_server`, `nvs_flash`, `esp-tls`, `esp_eth`, and `configurate`.

---

## Project Structure

```text
esp32-smart-environment/
│
├── components/
│   │
│   ├── BH1750/
│   │   └── BH1750 driver
│   │
│   ├── DHT22/
│   │   └── DHT22 driver
│   │
│   ├── configurate/
│   │   └── Hardware/configuration
│   │
│   ├── data_sensor/
│   │   └── Sensor data handling
│   │
│   └── web_server/
│       ├── web_server.c
│       └── web_server.h
│
├── main/
│   ├── mqtt.c
│   ├── dashboard.html
│   ├── Kconfig.projbuild
│   └── CMakeLists.txt
│
├── CMakeLists.txt
├── sdkconfig
└── .gitignore
```

The current repository contains separate components for BH1750, DHT22, configuration, sensor data, and the web server.

---

# Web Dashboard

The ESP32 hosts the dashboard directly through its HTTP server.

The HTML dashboard is embedded into the firmware and returned when the user accesses:

```text
http://<ESP32_IP>/
```

The dashboard provides:

* Temperature display
* Humidity display
* Ambient light display
* Soil moisture display
* Real-time environmental chart
* Water pump control
* Growth light control
* PWM control
* ESP32 system information
* Connection/status information

The dashboard uses Tailwind CSS, Chart.js, Font Awesome and JavaScript.

---

# REST API

The ESP32 HTTP server exposes several API endpoints.

## Get sensor data

```http
GET /api/sensors
```

Example response:

```json
{
  "temp": 28.5,
  "humi": 65.2,
  "light": 320.50,
  "soil": 78
}
```

The endpoint reads temperature and humidity from DHT22 and light intensity from BH1750.

---

## Control devices

```http
GET /api/control?device=relay1&state=1
```

Available devices:

```text
relay1
relay2
```

Example:

```text
/api/control?device=relay1&state=1
```

The ESP32 receives the command and changes the corresponding GPIO output.

---

## PWM Control

```http
GET /api/pwm?value=50
```

Example:

```text
/api/pwm?value=75
```

This sets the PWM duty cycle to approximately **75%**.

The firmware converts the 0–100% value into the configured LEDC timer resolution before updating the PWM output.

---

## ESP32 System Information

```http
GET /api/system
```

Example response:

```json
{
  "heap": 180000,
  "uptime": 120,
  "rssi": -55
}
```

The endpoint provides:

* Free heap memory
* System uptime
* Wi-Fi RSSI

---

# Data Flow

### Sensor monitoring

```text
DHT22
  │
  ├── Temperature
  └── Humidity
        │
        ▼
      ESP32
        │
        ▼
 /api/sensors
        │
        ▼
 Web Dashboard
```

### Light measurement

```text
BH1750
   │
   │ I2C
   ▼
 ESP32
   │
   ▼
 /api/sensors
   │
   ▼
 Dashboard
```

### Device control

```text
User
 │
 ▼
Web Dashboard
 │
 │ HTTP GET
 ▼
/api/control
 │
 ▼
ESP32
 │
 ▼
GPIO
 │
 ├── Relay 1
 └── Relay 2
```

### PWM control

```text
Web Dashboard
      │
      │ /api/pwm?value=XX
      ▼
    ESP32
      │
      ▼
   LEDC PWM
      │
      ▼
 Motor / Fan
```

---

# Wi-Fi

The ESP32 operates in **Wi-Fi Station mode**.

The firmware:

1. Initializes the network interface.
2. Creates the default Wi-Fi station interface.
3. Configures SSID and password.
4. Starts the Wi-Fi driver.
5. Waits for `IP_EVENT_STA_GOT_IP`.
6. Starts the HTTP server after the ESP32 obtains an IP address.

The code also handles Wi-Fi disconnection and retries the connection up to the configured maximum retry count.

---

# HTTP Server Lifecycle

The web server is intentionally tied to the Wi-Fi connection.

```text
ESP32 Boot
    │
    ▼
Initialize Hardware
    │
    ▼
Initialize NVS
    │
    ▼
Initialize Network
    │
    ▼
Connect to Wi-Fi
    │
    ├── Failed → Retry
    │
    ▼
Got IP
    │
    ▼
Start HTTP Server
    │
    ▼
Serve Dashboard + API
```

When Wi-Fi is disconnected, the HTTP server is stopped. When a new IP address is obtained, the server is started again.

---
# Dashboard Usage

After the ESP32 connects to Wi-Fi, the serial monitor will show its IP address.

For example:

```text
got ip:192.168.1.100
```

Open the address in a browser:

```text
http://192.168.1.100/
```

The dashboard will then communicate directly with the ESP32 HTTP server.
---

