# 🔒 IoT Smart Door System (Hệ thống Cửa Thông minh)

![Project Status](https://img.shields.io/badge/Status-Completed-success)
![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20Node--RED-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B%20%7C%20JavaScript-orange)
![License](https://img.shields.io/badge/License-MIT-green)

> Đồ án cuối kỳ môn Internet of Things (IoT). Hệ thống quản lý an ninh cửa thông minh tích hợp điều khiển từ xa, giám sát thời gian thực, lưu trữ Cloud và trợ lý ảo AI.

## 📖 Giới thiệu
**Smart Door System** là giải pháp an ninh gia đình toàn diện, cho phép người dùng giám sát và điều khiển cửa nhà thông qua giao diện Web Dashboard. Hệ thống kết hợp sức mạnh của vi điều khiển ESP32, giao thức MQTT tốc độ cao và trí tuệ nhân tạo để mang lại trải nghiệm nhà thông minh hiện đại.

### ✨ Tính năng nổi bật
* 🔐 **Điều khiển từ xa:** Mở/Khóa cửa (Unlock/Lock) qua Internet bất kể ở đâu.
* 👁️ **Giám sát thời gian thực:** Cập nhật trạng thái Cửa (Đóng/Mở) và Chuông cửa ngay lập tức.
* 🔔 **Hệ thống Chuông & Báo động:** Tích hợp chuông cửa và còi báo động khi có đột nhập (cạy cửa).
* ☁️ **Lưu trữ Cloud:** Tự động ghi log lịch sử hoạt động lên **ThingSpeak** (với cơ chế hàng đợi chống mất dữ liệu).
* 🤖 **AI Chatbot:** Trợ lý ảo tích hợp (sử dụng **Google Gemini API**) giúp trả lời câu hỏi về an ninh ngôi nhà bằng ngôn ngữ tự nhiên.
* 📊 **Dashboard trực quan:** Biểu đồ, đồng hồ đo và bảng lịch sử chi tiết.

## 🛠️ Kiến trúc hệ thống
Hệ thống hoạt động dựa trên mô hình Publish/Subscribe qua MQTT Broker.

```mermaid
graph TD
    User((Người dùng)) -->|Điều khiển/Hỏi| Dashboard[Node-RED Dashboard]
    Dashboard <-->|Logic & API| NodeRED(Node-RED Engine)
    NodeRED <-->|Pub/Sub| MQTT[HiveMQ Broker]
    MQTT <-->|WiFi| ESP32[ESP32 Controller]
    
    subgraph Hardware [Phần cứng]
        ESP32 -->|Điều khiển| Servo[Servo Khóa]
        ESP32 -->|Báo hiệu| Buzzer[Còi & LED]
        Sensor[Cảm biến cửa] -->|Input| ESP32
        Button[Nút chuông] -->|Input| ESP32
    end
    
    NodeRED <-->|Log dữ liệu| Cloud[ThingSpeak]
    NodeRED <-->|Hỏi đáp AI| AI[Google Gemini API]
