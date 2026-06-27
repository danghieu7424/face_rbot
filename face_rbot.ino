#include <LovyanGFX.hpp>

// ==============================================================
// PHÂN ĐOẠN 1: CẤU HÌNH PHẦN CỨNG LOVYANGFX (THEO TEST THỰC TẾ)
// ==============================================================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX(void) {
    auto cfg = _bus_instance.config();
    // Cấu hình SPI ESP32-S3 (FSPI / SPI2)
    cfg.spi_host = SPI2_HOST;  
    cfg.spi_mode = 3;          // Đã test: Mode 3 hoạt động tốt trên màn này
    cfg.freq_write = 20000000; // Đã test: Hạ tần số 20MHz tránh méo tín hiệu do dây cắm
    cfg.spi_3wire = false;     
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO; // Kích hoạt phần cứng DMA tự động

    // MAPPING CHÂN ĐÃ ĐƯỢC BẠN XÁC NHẬN
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

// Khởi tạo đối tượng màn hình và bộ đệm Sprite
LGFX tft;
LGFX_Sprite canvasSprite(&tft);

// ==============================================================
// PHÂN ĐOẠN 2: ĐỊNH NGHĨA TRẠNG THÁI VÀ LOGIC KHUÔN MẶT
// ==============================================================
struct FaceState {
  float eyeWidth;
  float eyeHeight;
  float eyeRadius;
  float mouthHeight; // 0 nghĩa là ẩn (idle)
  float mouthWidth;
};

// Tập các trạng thái khuôn mặt (Atomic States)
const FaceState stateNormal   = {40, 60, 15, 8,  40};
const FaceState stateSmile    = {45, 20, 20, 25, 60}; 
const FaceState stateIdle     = {40, 60, 15, 0,  0};  // Miệng co về 0 để biến mất
const FaceState stateTalkOpen = {38, 62, 10, 35, 35};
const FaceState stateTalkClose= {38, 62, 10, 4,  35};

FaceState currentFace = stateNormal;
FaceState targetFace = stateIdle; // Bắt đầu khởi động ở trạng thái Idle

float lerpSpeed = 0.3; // Tốc độ nội suy (Tăng lên 0.3 để bù lại framerate thấp hơn do SPI 20MHz)

// ==============================================================
// PHÂN ĐOẠN 3: LOGIC VẼ VÀ RENDER VÀO BỘ ĐỆM
// ==============================================================
void updateFaceLogic() {
  // Nội suy tuyến tính (Lerp)
  currentFace.eyeWidth    += (targetFace.eyeWidth    - currentFace.eyeWidth)    * lerpSpeed;
  currentFace.eyeHeight   += (targetFace.eyeHeight   - currentFace.eyeHeight)   * lerpSpeed;
  currentFace.eyeRadius   += (targetFace.eyeRadius   - currentFace.eyeRadius)   * lerpSpeed;
  currentFace.mouthHeight += (targetFace.mouthHeight - currentFace.mouthHeight) * lerpSpeed;
  currentFace.mouthWidth  += (targetFace.mouthWidth  - currentFace.mouthWidth)  * lerpSpeed;
}

void renderToScreen() {
  // 1. Xóa nền Sprite trong RAM (không chớp màn hình)
  canvasSprite.fillSprite(tft.color565(0, 0, 0)); // Nền đen

  // 2. Vẽ Mắt Trái & Phải
  uint32_t faceColor = tft.color565(0, 255, 255); // Màu Xanh Cyan
  
  // Mắt trái
  canvasSprite.fillRoundRect(40, 90 - (currentFace.eyeHeight/2), currentFace.eyeWidth, currentFace.eyeHeight, currentFace.eyeRadius, faceColor);
  // Mắt phải
  canvasSprite.fillRoundRect(200 - currentFace.eyeWidth, 90 - (currentFace.eyeHeight/2), currentFace.eyeWidth, currentFace.eyeHeight, currentFace.eyeRadius, faceColor);

  // 3. Xử lý logic Ẩn Miệng (Chỉ vẽ khi chiều cao > 0.5px)
  if (currentFace.mouthHeight > 0.5) {
    canvasSprite.fillRoundRect(120 - (currentFace.mouthWidth/2), 170 - (currentFace.mouthHeight/2), currentFace.mouthWidth, currentFace.mouthHeight, 6, faceColor);
  }

  // 4. Push toàn bộ Sprite lên màn hình bằng phần cứng DMA
  canvasSprite.pushSprite(0, 0);
}

// ==============================================================
// PHÂN ĐOẠN 4: VÒNG ĐỜI ARDUINO (MAIN LIFECYCLE)
// ==============================================================
unsigned long lastUpdate = 0;
unsigned long stateChangeTimer = 0;
int currentStateIndex = 0;

void setup() {
  Serial.begin(115200);
  
  // 1. Khởi tạo màn hình
  tft.init();
  tft.setRotation(0); // Xoay màn hình theo đúng file test của bạn
  tft.fillScreen(tft.color565(0, 0, 0)); // Xóa đen màn vật lý lần đầu

  // 2. CẤU HÌNH BỘ ĐỆM OPI PSRAM CHO ESP32-S3 N16R8
  canvasSprite.setPsram(true); 
  canvasSprite.setColorDepth(16); // 16-bit màu (RGB565)
  
  // Tạo bộ đệm vừa khớp màn hình 240x240
  if (canvasSprite.createSprite(240, 240) == nullptr) {
    Serial.println("LỖI: Không thể tạo Sprite! Kiểm tra lại cấu hình PSRAM trên Arduino IDE.");
    while (1); 
  }
  
  Serial.println("Khoi tao Face Voi LovyanGFX thanh cong!");
}

void loop() {
  unsigned long now = millis();

  // Task 1: Render Loop ~20 FPS (Mỗi 50ms)
  // [Góc khuất] Ở SPI 20MHz, đẩy 115KB data tốn ~46ms. Hạ FPS xuống 20 để tránh quá tải DMA
  if (now - lastUpdate >= 50) {
    lastUpdate = now;
    updateFaceLogic();
    renderToScreen();
  }

  // Task 2: Demo Chuyển đổi trạng thái ngẫu nhiên (mỗi 2.5 giây)
  if (now - stateChangeTimer > 2500) {
    stateChangeTimer = now;
    currentStateIndex = (currentStateIndex + 1) % 4;
    
    switch(currentStateIndex) {
      case 0: targetFace = stateNormal; break;
      case 1: targetFace = stateSmile; break;
      case 2: targetFace = stateTalkOpen; break;
      case 3: targetFace = stateIdle; break; // Ẩn miệng
    }
  }
}


