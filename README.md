# 🤖 SWARM Robotics System using ESP8266

## 📌 Overview
This project demonstrates a scalable SWARM robotics system built entirely with ESP8266 microcontrollers. The system enables distributed task execution and real-time data monitoring using ESP-NOW communication and cloud integration. One ESP8266 acts as the **master controller**, while others serve as **slave nodes** equipped with sensors for autonomous operation.

---

## ⚙️ Features

- 📡 **ESP-NOW Communication**: Low-latency, peer-to-peer protocol between ESP8266 devices.
- 🌡️ **Sensor Integration**: DHT11 for temperature & humidity, ultrasonic sensor for obstacle detection.
- ☁️ **Cloud Monitoring**: Real-time data pushed to the cloud for visualization and analysis.
- 📱 **IoT Control**: Blynk app used for remote control and system status updates.
- 🔋 **Low-Cost & Scalable**: Fully wireless, low-power system ideal for distributed robotic tasks.

---

## 🧩 System Architecture

- **Master Node (ESP8266)**  
  - Controls and monitors all slave nodes  
  - Sends and receives data via ESP-NOW  
  - Forwards sensor data to the cloud  

- **Slave Nodes (ESP8266)**  
  - Collect data from sensors  
  - Execute tasks based on commands from the master  

---

## 🛠️ Hardware Components

- ESP8266 (NodeMCU or equivalent) × 2+
- DHT11 Sensor (Temperature & Humidity)
- Ultrasonic Sensor (HC-SR04)
- Motor Driver (Optional)
- Power Supply (e.g., battery pack)
- Breadboard & Jumper Wires

---

## 📲 Software & Tools

- Arduino IDE
- ESP8266 Board Package
- Blynk IoT Platform
- Firebase (optional for cloud storage)
- GitHub for version control

---

## 🚀 Getting Started

1. Flash master and slave code to the ESP8266 devices.
2. Configure Blynk and Wi-Fi credentials in the code.
3. Power on devices — they will auto-connect via ESP-NOW.
4. Monitor data in Blynk or cloud platform and control actions remotely.

---



---

## 📚 Future Improvements

- Add path-planning and collision avoidance
- Extend to mesh networking
- Enable OTA firmware updates
- Use advanced sensors (e.g., GPS, IMU)





