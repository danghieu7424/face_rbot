#include <LovyanGFX.hpp>

/****
 * Chức năng: Lớp Cấu hình Phần cứng cho Màn hình LovyanGFX (ST7789 không CS).
 * Đầu vào: Không (Tự động map SPI/GPIO).
 * Đầu ra: Đối tượng cấu hình giao tiếp phần cứng DMA/SPI được tích hợp thẳng vào code.
 ****/
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX(void) {
    auto cfg = _bus_instance.config();
    // Cấu hình SPI ESP32-S3 (FSPI / SPI2)
    cfg.spi_host = SPI2_HOST;  
    cfg.spi_mode = 3;          // ĐỔI SANG MODE 3 (Nhiều màn hình ST7789 clone yêu cầu Mode 3 thay vì Mode 0)
    cfg.freq_write = 20000000; // HẠ TẦN SỐ XUỐNG 20MHz để tránh méo tín hiệu trên dây Jumper
    cfg.spi_3wire = false;     
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO; 

    // MAPPING CHÂN THEO HỘI THOẠI TRƯỚC (Rất quan trọng)
    cfg.pin_sclk = 10; 
    cfg.pin_mosi = 11; 
    cfg.pin_miso = -1; // Màn hình chỉ nhận dữ liệu, không truyền lại
    cfg.pin_dc   = 13;  
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    auto p_cfg = _panel_instance.config();
    p_cfg.pin_cs   = -1;   // Cấu hình đặc biệt cho màn hình không chân CS
    p_cfg.pin_rst  = 12;   
    p_cfg.pin_busy = -1;
    
    // Cấu hình phân giải và màu sắc hiển thị
    p_cfg.panel_width  = 240;
    p_cfg.panel_height = 240;
    p_cfg.offset_x = 0;    
    p_cfg.offset_y = 0;    
    p_cfg.invert = true;   // Chống âm bản ST7789
    p_cfg.rgb_order = false; 

    _panel_instance.config(p_cfg);
    setPanel(&_panel_instance);
  }
};

LGFX tft;

/****
 * Chức năng: Khởi tạo màn hình LovyanGFX và in test.
 * Đầu vào: Không.
 * Đầu ra: Màn hình đen, text trắng.
 ****/
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("LovyanGFX Test OK!", 10, 10, 4); 
}

/****
 * Chức năng: Vòng lặp đảo màu test màn hình với LGFX.
 * Đầu vào: Không.
 * Đầu ra: Log Serial và thay đổi màu TFT luân phiên 1 giây.
 ****/
void loop() {
  tft.fillScreen(TFT_RED);
  tft.drawString("LGFX - RED", 10, 50, 4);
  Serial.println("LGFX - RED");
  delay(1000);
  
  tft.fillScreen(TFT_GREEN);
  tft.drawString("LGFX - GREEN", 10, 50, 4);
  Serial.println("LGFX - GREEN");
  delay(1000);
  
  tft.fillScreen(TFT_BLUE);
  tft.drawString("LGFX - BLUE", 10, 50, 4);
  Serial.println("LGFX - BLUE");
  delay(1000);
}


