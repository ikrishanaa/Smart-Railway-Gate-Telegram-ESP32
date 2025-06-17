# 🚦 Smart Railway Gate with ESP32 & Telegram Bot

A real-time, obstacle-aware Smart Railway Gate System powered by an **ESP32**, with **dual servos**, **ultrasonic sensors**, **LED indicators**, and a **Telegram bot interface** for remote control and monitoring.

> Built to reduce human error and increase safety — especially in rural or unmanned railway crossings.

---

## 📸 Demo

_Add your GIF here in `/images/demo.gif`_  
Example:  
![demo](images/demo.gif)

---

## 🔧 Features

- 🟢 Opens & closes railway gate automatically with servo motors
- 📡 Controlled via Telegram bot using commands like `/open`, `/close`, `/status`
- 🚧 Avoids closing gate if obstacle is detected via ultrasonic sensors
- 📢 Buzzer with increasing frequency during motion
- 🟢/🔴 LEDs indicate gate status (Opening/Closing)
- 📩 Sends alerts on Telegram (e.g., "Object detected")

---

## 📦 Components Used

| Component                    | Quantity | Description                                      |
|-----------------------------|----------|--------------------------------------------------|
| ESP32                       | 1        | Main controller board                            |
| Servo Motors (MG90s/SG90)   | 2        | To move the gates up/down                        |
| Ultrasonic Sensors (HC-SR04)| 2        | For obstacle detection on both sides             |
| Buzzer                      | 1        | For warning beep (increasing pitch)              |
| LEDs (Red + Green)          | 2        | Red = Gate Closing/Closed, Green = Opening/Open  |
| 220Ω Resistor               | 2        | Current limiter for LEDs                         |
| 100μF Capacitor             | 1        | Power stabilizer for servos                      |
| Breadboard + Wires          | Many     | For prototyping                                  |
| Internet-connected WiFi     | 1        | Required for ESP32 to talk to Telegram           |

---

## 💬 Telegram Bot Setup

1. **Create Bot**
   - Open Telegram → Search `@BotFather`
   - Type `/newbot`
   - Give it a name and username
   - You'll get a **BOT TOKEN** like:
     ```
     123456789:AAHfkGuwSdf1asfasdf...
     ```

2. **Get Chat ID**
   - Open your Telegram bot and **send any message**
   - Visit:  
     [https://api.telegram.org/botYOUR_TOKEN/getUpdates](https://api.telegram.org/botYOUR_TOKEN/getUpdates)
   - Look for:
     ```json
     "chat": { "id": 123456789, ... }
     ```
   - That `id` is your **Chat ID**

---

## 🧠 Working Principle

- When `/open` or `/close` is received on Telegram:
  - Servo moves slowly (2.5–3.8s) to open or close the gate
  - Buzzer starts with slow beeps and increases frequency
  - LED glows:
    - 🔴 while gate is closing or closed
    - 🟢 while gate is opening or open
- Before closing:
  - Both ultrasonic sensors check for objects within 20cm
  - If object found → `Gate doesn't close`, Telegram sends `Object detected`
- `/status` shows current gate state and whether an object is detected.

---

## 🪛 Wiring Overview

| ESP32 Pin | Component           | Notes                                      |
|-----------|---------------------|--------------------------------------------|
| D2        | Servo 1             | Left gate                                  |
| D4        | Servo 2             | Right gate                                 |
| D5        | Buzzer              | PWM or tone output                         |
| D12       | Red LED             | With 220Ω resistor                         |
| D13       | Green LED           | With 220Ω resistor                         |
| D14       | Ultrasonic 1 - TRIG | Front sensor                               |
| D27       | Ultrasonic 1 - ECHO |                                            |
| D25       | Ultrasonic 2 - TRIG | Back sensor                                |
| D26       | Ultrasonic 2 - ECHO |                                            |
| GND       | All grounds         | Common GND                                  |
| 3.3V/5V   | Power supply        | Via regulated USB or external source       |
| 100μF Cap | Servo motor supply  | Connected between Vcc & GND near servos    |

> Note: Use capacitor to prevent servo resets due to voltage drops.

---

## ⚙️ Installation

1. Install **Arduino IDE**
2. Install ESP32 board via Board Manager
3. Install required libraries:
   - `WiFi.h`
   - `HTTPClient.h`
   - `Servo.h`
   - `NewPing.h`
4. Open `/src/main.ino` and update these:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   String botToken = "YOUR_BOT_TOKEN";
   String chatId = "YOUR_CHAT_ID";
