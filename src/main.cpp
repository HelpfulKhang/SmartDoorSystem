#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// --- CẤU HÌNH WIFI & MQTT ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

// --- TOPIC MQTT (Thay MSSV của bạn vào đây) ---
const char* topic_status   = "23127203-23127460/door/status";   // Gửi lên: OPEN / CLOSED
const char* topic_bell     = "23127203-23127460/door/doorbell"; // Gửi lên: RING
const char* topic_command  = "23127203-23127460/door/command";  // Nhận về: UNLOCK / LOCK

// --- ĐỊNH NGHĨA CHÂN ---
#define PIN_DOOR_SENSOR 4   // Slide Switch (Cảm biến cửa)
#define PIN_BELL_BUTTON 5   // Nút chuông
#define PIN_SERVO       13  // Servo chốt khóa
#define PIN_BUZZER      14  // Còi
#define PIN_RGB_RED     18  
#define PIN_RGB_GREEN   19  
#define PIN_RGB_BLUE    21  

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Servo myServo;

// Biến trạng thái
int lastDoorState = -1; 
int lastBellState = HIGH;
bool isLocked = true; // Theo dõi trạng thái chốt (Logic nội bộ)

void wifiConnect() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" Connected!");
}

// --- HÀM XỬ LÝ LỆNH TỪ WEB (CHỈ ĐIỀU KHIỂN SERVO) ---
void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  Serial.print("CMD Received: "); Serial.println(msg);

  if (String(topic) == topic_command) {
    
    // Lệnh MỞ KHÓA (Rút chốt)
    if (msg == "UNLOCK") {
      myServo.write(90); // Quay servo 90 độ
      isLocked = false;
      Serial.println("Action: Unlocked Servo");
      // Bíp nhẹ báo hiệu đã nhận lệnh
      digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
    }
    
    // Lệnh KHÓA (Đóng chốt)
    else if (msg == "LOCK") {
      // Chỉ cho phép khóa khi cửa đang đóng vật lý
      if (digitalRead(PIN_DOOR_SENSOR) == LOW) {
        myServo.write(0); // Quay servo về 0 độ
        isLocked = true;
        Serial.println("Action: Locked Servo");
        digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
      } else {
        Serial.println("Warning: Cannot Lock - Door is Open!");
        // Nháy đỏ báo lỗi không khóa được
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

  // Khởi tạo Servo (Mặc định khóa)
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
      mqttClient.subscribe(topic_command); // Chỉ nghe lệnh điều khiển chốt
    } else {
      delay(5000);
    }
  }
}

void loop() {
  if (!mqttClient.connected()) mqttConnect();
  mqttClient.loop();

  // --- 1. XỬ LÝ CẢM BIẾN CỬA (GỬI OPEN/CLOSED) ---
  int currentDoorState = digitalRead(PIN_DOOR_SENSOR);
  
  if (currentDoorState != lastDoorState) {
    delay(50); // Chống dội
    if (digitalRead(PIN_DOOR_SENSOR) == currentDoorState) {
      
      if (currentDoorState == HIGH) {
        // Cửa bị mở ra
        Serial.println("Status: OPEN");
        mqttClient.publish(topic_status, "OPEN");
        
        // Đèn Đỏ: Cửa đang mở
        digitalWrite(PIN_RGB_RED, HIGH); 
        digitalWrite(PIN_RGB_GREEN, LOW);
        
        // Nếu chốt đang khóa mà cửa mở -> Báo động giả lập (Optional)
        if (isLocked) Serial.println("ALARM: Forced Open!");
      } 
      else {
        // Cửa đóng lại
        Serial.println("Status: CLOSED");
        mqttClient.publish(topic_status, "CLOSED");
        
        // Đèn Xanh: Cửa đang đóng
        digitalWrite(PIN_RGB_RED, LOW); 
        digitalWrite(PIN_RGB_GREEN, HIGH);
      }
      lastDoorState = currentDoorState;
    }
  }

  // --- 2. XỬ LÝ CHUÔNG CỬA ---
  int currentBellState = digitalRead(PIN_BELL_BUTTON);
  if (lastBellState == HIGH && currentBellState == LOW) {
    Serial.println("Event: Doorbell Ring");
    mqttClient.publish(topic_bell, "RING");
    
    // Hiệu ứng bấm chuông (Nháy xanh dương)
    digitalWrite(PIN_RGB_BLUE, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(200);
    digitalWrite(PIN_RGB_BLUE, LOW);
    digitalWrite(PIN_BUZZER, LOW);
  }
  lastBellState = currentBellState;
}