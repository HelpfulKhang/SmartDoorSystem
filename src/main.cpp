#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// --- CẤU HÌNH WIFI & MQTT ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

// --- TOPIC MQTT ---
const char* topic_status   = "23127203-23127460/door/status";
const char* topic_bell     = "23127203-23127460/door/doorbell";
const char* topic_command  = "23127203-23127460/door/command";

// --- ĐỊNH NGHĨA CHÂN ---
#define PIN_DOOR_SENSOR 4   // Input: Cảm biến
#define PIN_BELL_BUTTON 5   // Input: Nút chuông
#define PIN_SERVO       13  // Output: Servo
#define PIN_BUZZER      14  // Output: Còi
#define PIN_RGB_RED     18  
#define PIN_RGB_GREEN   19  
#define PIN_RGB_BLUE    21  

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Servo myServo;

// Biến trạng thái
int lastBellState = HIGH;      
int lastDoorSensorState = -1;  
unsigned long lastBellTime = 0; // [MỚI] Thời điểm bấm chuông cuối cùng

void wifiConnect() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    delay(100);
  }
}

void setColor(bool red, bool green, bool blue) {
  digitalWrite(PIN_RGB_RED, red);
  digitalWrite(PIN_RGB_GREEN, green);
  digitalWrite(PIN_RGB_BLUE, blue);
}

void updateDoorStatus(bool isOpen) {
  mqttClient.publish(topic_status, isOpen ? "OPEN" : "CLOSED");
}

void openDoor() {
    Serial.println("CMD: OPEN SERVO");
    myServo.write(90); 
    beep(1);
}

void closeDoor() {
    Serial.println("CMD: CLOSE SERVO");
    myServo.write(0); 
    beep(1);
}

void mqttConnect() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "23127203-23127460"; 
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected!");
      mqttClient.subscribe(topic_command);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  
  if (String(topic) == topic_command) {
    if (msg == "OPEN") openDoor();
    else if (msg == "CLOSED" || msg == "LOCK") closeDoor();
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
  pinMode(PIN_BELL_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RGB_RED, OUTPUT);
  pinMode(PIN_RGB_GREEN, OUTPUT);
  pinMode(PIN_RGB_BLUE, OUTPUT);

  myServo.attach(PIN_SERVO);
  myServo.write(0); 
  
  setColor(0, 1, 0); 

  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) mqttConnect();
  mqttClient.loop();

  unsigned long now = millis(); // Lấy thời gian hiện tại

  // --- 1. XỬ LÝ NÚT CHUÔNG ---
  int currentBellState = digitalRead(PIN_BELL_BUTTON);
  
  if (lastBellState == HIGH && currentBellState == LOW) {
    Serial.println("Ring Ring!");
    mqttClient.publish(topic_bell, "RING");
    
    // Hiệu ứng bấm chuông
    setColor(0, 0, 1); // Blue ON
    beep(1);
    delay(200); 
    
    // [FIX 1]: Trả lại màu dựa trên BIẾN NHỚ (lastDoorSensorState)
    // Thay vì đọc lại chân Digital (có thể đang bị nhiễu)
    if(lastDoorSensorState == HIGH) setColor(1, 0, 0); // Đỏ (đang mở)
    else setColor(0, 1, 0); // Xanh (đang đóng)

    // [FIX 2]: Ghi lại thời điểm bấm chuông để chặn cảm biến cửa 1 chút
    lastBellTime = millis(); 
  }
  lastBellState = currentBellState;


  // --- 2. XỬ LÝ CẢM BIẾN CỬA ---
  // [FIX 3]: Chỉ xử lý cảm biến cửa nếu ĐÃ QUA 500ms kể từ lần bấm chuông cuối cùng
  // Điều này giúp tránh việc rung tay khi bấm chuông làm cảm biến cửa bị nhảy sai
  if (now - lastBellTime > 500) {
      
      int currentDoorState = digitalRead(PIN_DOOR_SENSOR);
      
      if (currentDoorState != lastDoorSensorState) {
          // Delay nhỏ để chống dội tín hiệu (Debounce) trước khi quyết định
          delay(50); 
          // Đọc lại lần nữa cho chắc ăn
          if (digitalRead(PIN_DOOR_SENSOR) == currentDoorState) {
              
              if (currentDoorState == HIGH) {
                  Serial.println("Sensor: Cửa MỞ");
                  updateDoorStatus(true);
                  setColor(1, 0, 0); 
              } else {
                  Serial.println("Sensor: Cửa ĐÓNG");
                  updateDoorStatus(false);
                  setColor(0, 1, 0); 
              }
              lastDoorSensorState = currentDoorState; 
          }
      }
  }
}