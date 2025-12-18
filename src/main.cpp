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
#define PIN_DOOR_SENSOR 4   
#define PIN_BELL_BUTTON 5   
#define PIN_SERVO       13  
#define PIN_BUZZER      14  
#define PIN_RGB_RED     18  
#define PIN_RGB_GREEN   19  
#define PIN_RGB_BLUE    21  

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Servo myServo;

// Biến trạng thái
int lastDoorState = 1; 
int lastBellState = LOW;
bool isLocked = true; 

// [MỚI] Biến để chặn nhiễu cảm biến khi bấm chuông
unsigned long lastBellTime = 0; 

void wifiConnect() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" Connected!");
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  Serial.print("CMD Received: "); Serial.println(msg);

  if (String(topic) == topic_command) {
    if (msg == "UNLOCK") {
      myServo.write(90); 
      isLocked = false;
      Serial.println("Action: Unlocked Servo");
      digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
    }
    else if (msg == "LOCK") {
      if (digitalRead(PIN_DOOR_SENSOR) == LOW) {
        myServo.write(0); 
        isLocked = true;
        Serial.println("Action: Locked Servo");
        digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
      } else {
        Serial.println("Warning: Cannot Lock - Door is Open!");
        digitalWrite(PIN_RGB_RED, HIGH); delay(200); digitalWrite(PIN_RGB_RED, LOW);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Cấu hình chân
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
  pinMode(PIN_BELL_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RGB_RED, OUTPUT);
  pinMode(PIN_RGB_GREEN, OUTPUT);
  pinMode(PIN_RGB_BLUE, OUTPUT);

  // Khởi tạo Servo
  myServo.attach(PIN_SERVO);
  myServo.write(0); 
  
  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
}

void mqttConnect() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "23127203-23127460";
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected!");
      mqttClient.subscribe(topic_command); 
    } else {
      delay(5000);
    }
  }
}

void loop() {
  if (!mqttClient.connected()) mqttConnect();
  mqttClient.loop();

  // Lấy thời gian hiện tại
  unsigned long now = millis();

  // --- 1. XỬ LÝ CHUÔNG CỬA (Ưu tiên xử lý trước) ---
  int currentBellState = digitalRead(PIN_BELL_BUTTON);
  if (lastBellState == HIGH && currentBellState == LOW) {
    Serial.println("Event: Doorbell Ring");
    mqttClient.publish(topic_bell, "RING");
    
    // [QUAN TRỌNG] Ghi lại thời gian bấm chuông
    lastBellTime = now; 

    // Hiệu ứng bấm chuông
    digitalWrite(PIN_RGB_BLUE, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(200);
    digitalWrite(PIN_RGB_BLUE, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    
    // [QUAN TRỌNG] Sau khi chuông reo xong, trả lại màu đèn cũ ngay lập tức
    // để tránh bị sai màu
    if (digitalRead(PIN_DOOR_SENSOR) == HIGH) {
        digitalWrite(PIN_RGB_RED, HIGH); // Nếu cửa đang mở -> Đỏ
    } else {
        digitalWrite(PIN_RGB_GREEN, HIGH); // Nếu cửa đang đóng -> Xanh
    }
  }
  lastBellState = currentBellState;

  // --- 2. XỬ LÝ CẢM BIẾN CỬA (CÓ ĐIỀU KIỆN CHẶN) ---
  // Chỉ xử lý cảm biến cửa nếu đã qua 0.5 giây kể từ lần bấm chuông cuối cùng
  if (now - lastBellTime > 500) { 
      
      int currentDoorState = digitalRead(PIN_DOOR_SENSOR);
      
      if (currentDoorState != lastDoorState) {
        delay(50); // Chống dội
        if (digitalRead(PIN_DOOR_SENSOR) == currentDoorState) {
          
          if (currentDoorState == HIGH) {
            // Cửa MỞ
            Serial.println("Status: OPEN");
            mqttClient.publish(topic_status, "OPEN");
            digitalWrite(PIN_RGB_RED, HIGH); 
            digitalWrite(PIN_RGB_GREEN, LOW);
            if (isLocked) Serial.println("ALARM: Forced Open!");
          } 
          else {
            // Cửa ĐÓNG
            Serial.println("Status: CLOSED");
            mqttClient.publish(topic_status, "CLOSED");
            digitalWrite(PIN_RGB_RED, LOW); 
            digitalWrite(PIN_RGB_GREEN, HIGH);
          }
          lastDoorState = currentDoorState;
        }
      }
  }
}