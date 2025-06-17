# 🚦 Smart Railway Gate System using ESP32 + Telegram

This project is a smart IoT-based railway crossing system that allows gate control using a Telegram bot. It includes safety features like obstacle detection using an ultrasonic sensor, buzzer alerts, and real-time command handling.

## 🧠 Features

- Remote gate control via Telegram (/open, /close, /status)
- Obstacle detection before closing the gate
- Gradual gate movement using servo
- Dynamic buzzer alerts
- LCD display for local status

## 🛠 Hardware Used

| Component            | Quantity |
|----------------------|----------|
| ESP32                | 1        |
| Servo Motor (SG90/MG995) | 1        |
| Ultrasonic Sensor (HC-SR04) | 1        |
| Buzzer               | 1        |
| I2C LCD (16x2)       | 1        |
| Power Supply         | 1        |

## 📲 Telegram Commands

| Command  | Function                |
|----------|-------------------------|
| `/start` | Initialize bot greeting |
| `/open`  | Open the gate           |
| `/close` | Close the gate          |
| `/status`| Get system status       |

## ⚙️ Circuit Diagram

![Circuit](hardware/circuit_diagram.jpg)

## 🚀 Getting Started

1. Clone the repo  
   ```bash
   git clone https://github.com/yourusername/Smart-Railway-Gate-Telegram-ESP32.git
