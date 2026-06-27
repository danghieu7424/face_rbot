/****
 * Chức năng: Khởi tạo và kiểm tra đèn LED trên GPIO01 kết hợp gửi dữ liệu debug qua Serial.
 * Đầu vào: Không.
 * Đầu ra: Trạng thái bật/tắt LED và log Serial.
 ****/
void setup() {
  // Cấu hình Serial với tốc độ baud 115200 (phù hợp với mặc định của `cargo espflash monitor`)
  Serial.begin(115200);
  
  // Khởi tạo chân GPIO01 làm ngõ ra điều khiển LED (căn cứ theo cấu hình phần cứng thử nghiệm)
  pinMode(1, OUTPUT); 
}

/****
 * Chức năng: Vòng lặp chính đảo trạng thái LED liên tục để kiểm tra mạch.
 * Đầu vào: Không.
 * Đầu ra: Chuyển đổi trạng thái HIGH/LOW trên chân GPIO01 và in log.
 ****/
void loop() {
  digitalWrite(1, HIGH);
  Serial.println("LED ON - GPIO01");
  // Độ trễ 1000ms để quan sát trực quan bằng mắt (chu kỳ chuẩn test)
  delay(1000);
  
  digitalWrite(1, LOW);
  Serial.println("LED OFF - GPIO01");
  delay(1000);
}
