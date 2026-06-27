#include <TFT_eSPI.h> 

// Khởi tạo đối tượng giao tiếp màn hình (Các chân SPI được cấu hình ngầm trong User_Setup.h của thư viện)
TFT_eSPI tft = TFT_eSPI(); 

/****
 * Chức năng: Khởi tạo màn hình TFT và vẽ nội dung kiểm tra cơ bản.
 * Đầu vào: Không.
 * Đầu ra: Màn hình TFT khởi động, đổ màu nền và in text xác nhận giao tiếp SPI thành công.
 ****/
void setup() {
  // Cấu hình Serial baudrate 115200 (Đảm bảo đồng bộ với cấu hình nạp và cargo monitor)
  Serial.begin(115200);
  
  // Bắt đầu khởi tạo quy trình IC điều khiển màn hình
  tft.init();
  
  // Xoay màn hình 90 độ (Lưu ý: Hướng xoay phụ thuộc vào thiết kế cơ khí cố định màn hình)
  tft.setRotation(1);
  
  // Xóa rác bộ đệm hiển thị, đặt màn hình đen chuẩn bị test
  tft.fillScreen(TFT_BLACK);
  
  // Cài đặt hiển thị chữ trắng, nền đen chống răng cưa
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  tft.drawString("TFT_eSPI Test OK!", 10, 10, 4); 
}

/****
 * Chức năng: Vòng lặp chính đảo màu màn hình để kiểm tra Tần số quét (Refresh Rate) và Điểm ảnh chết (Dead Pixel).
 * Đầu vào: Không.
 * Đầu ra: Màn hình chớp đổi màu RGB theo chu kỳ 1 giây và log trạng thái ra Serial.
 ****/
void loop() {
  // Fill màu Đỏ (Phục vụ quá trình QA kiểm tra hở sáng hoặc Dead Pixel mảng Đỏ)
  tft.fillScreen(TFT_RED);
  tft.drawString("TEST RED", 10, 50, 4);
  Serial.println("TFT - RED");
  delay(1000);
  
  // Fill màu Xanh Lá
  tft.fillScreen(TFT_GREEN);
  tft.drawString("TEST GREEN", 10, 50, 4);
  Serial.println("TFT - GREEN");
  delay(1000);
  
  // Fill màu Xanh Dương
  tft.fillScreen(TFT_BLUE);
  tft.drawString("TEST BLUE", 10, 50, 4);
  Serial.println("TFT - BLUE");
  delay(1000);
}

