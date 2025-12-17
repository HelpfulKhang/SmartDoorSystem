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
#define PIN_DOOR_SENSOR 4   // Input: Cảm biến (Slide Switch)
#define PIN_BELL_BUTTON 5   // Input: Nút chuông (Button)
#define PIN_SERVO       13  // Output: Servo
#define PIN_BUZZER      14  // Output: Còi

// Chân cho LED RGB
#define PIN_RGB_RED     18  // Chân R
#define PIN_RGB_GREEN   19  // Chân G
#define PIN_RGB_BLUE    21  // Chân B

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Servo myServo;

// Biến trạng thái
int lastBellState = HIGH;      // Trạng thái nút chuông cũ
int lastDoorSensorState = -1;  // Trạng thái cảm biến cũ (-1 để cập nhật ngay lần đầu)

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

// --- HÀM ĐIỀU KHIỂN MÀU RGB ---
void setColor(bool red, bool green, bool blue) {
  digitalWrite(PIN_RGB_RED, red);
  digitalWrite(PIN_RGB_GREEN, green);
  digitalWrite(PIN_RGB_BLUE, blue);
}

void updateDoorStatus(bool isOpen) {
  mqttClient.publish(topic_status, isOpen ? "OPEN" : "CLOSED");
}

// --- HÀM ĐIỀU KHIỂN SERVO (CHỈ QUAY MOTOR) ---
void openDoor() {
    Serial.println("CMD: OPEN SERVO");
    myServo.write(90); // Mở chốt
    beep(1);
    // Lưu ý: Không đổi màu đèn hay gửi MQTT ở đây nữa
    // Việc đó để Cảm biến ở trong Loop lo.
}

void closeDoor() {
    Serial.println("CMD: CLOSE SERVO");
    myServo.write(0); // Khóa chốt
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

  // Cấu hình chân
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP); // Cảm biến kéo cao
  pinMode(PIN_BELL_BUTTON, INPUT_PULLUP); // Nút chuông kéo cao (cho ổn định)
  pinMode(PIN_BUZZER, OUTPUT);
  
  // Cấu hình 3 chân màu của RGB là OUTPUT
  pinMode(PIN_RGB_RED, OUTPUT);
  pinMode(PIN_RGB_GREEN, OUTPUT);
  pinMode(PIN_RGB_BLUE, OUTPUT);

  // Khởi tạo Servo
  myServo.attach(PIN_SERVO);
  myServo.write(0); 
  
  // Mặc định: Đóng cửa -> Sáng màu XANH LÁ
  setColor(0, 1, 0); 

  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) mqttConnect();
  mqttClient.loop();

  // --- 1. XỬ LÝ CẢM BIẾN CỬA (Logic Quan Trọng Nhất) ---
  int currentDoorState = digitalRead(PIN_DOOR_SENSOR);
  
  // Nếu trạng thái cảm biến thay đổi so với lần trước
  if (currentDoorState != lastDoorSensorState) {
      if (currentDoorState == HIGH) {
          // HIGH = Hở mạch = Cửa Mở -> Đèn Đỏ + Gửi "OPEN"
          Serial.println("Sensor: Cửa MỞ");
          updateDoorStatus(true);
          setColor(1, 0, 0); 
      } else {
          // LOW = Chạm nhau = Cửa Đóng -> Đèn Xanh + Gửi "CLOSED"
          Serial.println("Sensor: Cửa ĐÓNG");
          updateDoorStatus(false);
          setColor(0, 1, 0); 
      }
      lastDoorSensorState = currentDoorState; // Lưu lại
      delay(100); // Chống dội (Debounce)
  }

  // --- 2. XỬ LÝ NÚT CHUÔNG ---
  int currentBellState = digitalRead(PIN_BELL_BUTTON);
  
  // Phát hiện nhấn nút (Chuyển từ Cao xuống Thấp do INPUT_PULLUP)
  if (lastBellState == HIGH && currentBellState == LOW) {
    Serial.println("Ring Ring!");
    mqttClient.publish(topic_bell, "RING");
    
    // Hiệu ứng nháy màu xanh dương khi bấm chuông
    setColor(0, 0, 1); // Blue ON
    beep(1);
    delay(200);
    
    // Trả lại màu trạng thái cũ dựa trên Cảm biến
    if(digitalRead(PIN_DOOR_SENSOR) == HIGH) setColor(1, 0, 0); // Đỏ nếu đang mở
    else setColor(0, 1, 0); // Xanh nếu đang đóng
  }
  lastBellState = currentBellState;
}