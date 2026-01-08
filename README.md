# 🔒 IoT Smart Door System

![Project Status](https://img.shields.io/badge/Status-Completed-success)
![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20Node--RED-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B%20%7C%20JavaScript-orange)
![License](https://img.shields.io/badge/License-MIT-green)

> **Internet of Things (IoT) Final Project.** A comprehensive smart home security system featuring remote access control, real-time monitoring, cloud data logging, and an AI-powered assistant.

## 📖 Overview
The **Smart Door System** is designed to provide secure and convenient access control for modern homes. Leveraging the **ESP32** microcontroller, **MQTT** protocol, and **Node-RED**, the system allows users to monitor door status, control the lock remotely, and track activity logs via a cloud database. Additionally, it integrates **Google Gemini AI** to act as a virtual home security assistant.

### ✨ Key Features
* 🔐 **Remote Control:** Lock and Unlock the door from anywhere via the Web Dashboard.
* 👁️ **Real-time Monitoring:** Instant visualization of Door Status (Open/Closed) and Doorbell activity.
* 🔔 **Security & Alerts:** Integrated doorbell and forced-entry alarm (buzzer & LED indicators).
* ☁️ **Cloud Logging:** Automatically logs all events to **ThingSpeak** (implemented with a delay queue to handle rate limits).
* 🤖 **AI Chatbot:** An intelligent assistant (powered by **Google Gemini**) that answers natural language questions about home security status.
* 📊 **Interactive Dashboard:** Includes real-time charts, gauges, and historical data tables.

## 🛠️ System Architecture
The system follows a Publish/Subscribe model using an MQTT Broker for low-latency communication.

```mermaid
---
config:
  theme: dark
  layout: fixed
---
flowchart TD
    
    %% --- CLASS DEFINITIONS ---
    classDef hardware fill:#4a1414,stroke:#ff9999,stroke-width:2px,color:#ffffff
    classDef server fill:#0d3d56,stroke:#80d8ff,stroke-width:2px,color:#ffffff
    classDef cloud fill:#1b4d2e,stroke:#a5d6a7,stroke-width:2px,color:#ffffff
    classDef user fill:#5e4b12,stroke:#fff59d,stroke-width:2px,color:#ffffff
    classDef device fill:#424242,stroke:#ffffff,stroke-width:1px,color:#ffffff

    %% --- BLOCK 1: UI & CLOUD (Application Layer) ---
    User(("User")):::user
    
    subgraph Cloud_Layer ["Cloud Services"]
        style Cloud_Layer fill:#263238,stroke:#546e7a,color:#ffffff
        ThingSpeak[("ThingSpeak DB")]:::cloud
        OpenAI("Gemini AI API"):::cloud
    end

    subgraph System_Core ["Core System"]
        style System_Core fill:#263238,stroke:#546e7a,color:#ffffff
        Dashboard["Web Dashboard<br>UI"]:::server
        NodeRED("Node-RED Engine"):::server
        MQTT["HiveMQ Broker"]:::server
    end

    %% --- BLOCK 2: HARDWARE (Physical Layer) ---
    subgraph Hardware_Layer ["IoT Hardware"]
        style Hardware_Layer fill:#263238,stroke:#546e7a,color:#ffffff
        
        %% Inputs
        ReedSwitch["Door Sensor"]:::device
        Button["Doorbell Button"]:::device
        
        %% Controller
        ESP32["ESP32 Controller"]:::hardware
        
        %% Outputs
        Servo["Servo Lock"]:::device
        Buzzer["Buzzer & LED"]:::device
    end

    %% --- CONNECTIONS ---
    User -- "1. Click/View" --> Dashboard
    Dashboard -- "2. Command" --> NodeRED
    NodeRED -- "3. Pub" --> MQTT
    MQTT -- "4. Sub" --> ESP32
    ESP32 -- "5. Rotate" --> Servo
    ESP32 -. Alert .-> Buzzer
    ReedSwitch -- Open/Close --> ESP32
    Button -- Press --> ESP32
    ESP32 -- Pub --> MQTT
    MQTT -- Sub --> NodeRED
    NodeRED -- Update UI --> Dashboard
    NodeRED -- Write Log --> ThingSpeak
    ThingSpeak -- Read JSON --> NodeRED
    User -- Ask AI --> Dashboard
    Dashboard -- Send Ctx --> NodeRED
    NodeRED -- Request --> OpenAI
    OpenAI -- Response --> NodeRED
    NodeRED -- Show Ans --> Dashboard

    %% --- ALIGNMENT ---
    Dashboard ~~~ ThingSpeak
    ReedSwitch ~~~ ESP32
    Button ~~~ ESP32

    %% --- STYLE ---
    linkStyle default stroke:#00e5ff,stroke-width:2px,fill:none
