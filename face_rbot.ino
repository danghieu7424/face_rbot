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

// Khởi tạo đối tượng màn hình và 2 bộ đệm Sprite (Main + Eye Sub-Sprite)
LGFX tft;
LGFX_Sprite canvasSprite(&tft);
LGFX_Sprite eyeSprite(&tft);

// ==============================================================
// PHÂN ĐOẠN 2: ĐỊNH NGHĨA TRẠNG THÁI VÀ LOGIC KHUÔN MẶT
// ==============================================================
struct FaceState {
  float eyeShapeType; // 0: Oval, 1: Bán nguyệt trên, 2: Bán nguyệt dưới, 3: Oval phẳng đáy
  float eyeWidth;
  float eyeHeight;
  float eyeRadius;
  float eyeAngle;
  float glowSize;
  float innerShadow;
  float mouthHeight; 
  float mouthWidth;
  float mouthGlowSize;
  float mouthInnerShadow;
};

// Tập các trạng thái khuôn mặt (Atomic States)
const FaceState stateNormal   = {0, 40, 50, 20, 7,  4, 14, 8,  40, 2, 4};
const FaceState stateSmile    = {1, 45, 20, 20, 10, 4, 14, 25, 60, 2, 4}; 
const FaceState stateIdle     = {0, 40, 50, 20, 7,  4, 14, 0,  0,  0, 0};  
const FaceState stateTalkOpen = {0, 38, 62, 10, 5,  4, 14, 35, 35, 2, 4};
const FaceState stateTalkClose= {0, 38, 62, 10, 5,  4, 14, 4,  35, 2, 4};

FaceState currentFace = stateNormal;
FaceState targetFace = stateIdle; // Bắt đầu khởi động ở trạng thái Idle

float lerpSpeed = 0.3; // Tốc độ nội suy (Tăng lên 0.3 để bù lại framerate thấp hơn do SPI 20MHz)

// ==============================================================
// PHÂN ĐOẠN 3: LOGIC VẼ VÀ RENDER VÀO BỘ ĐỆM
// ==============================================================
void updateFaceLogic() {
  // ShapeType chuyển đổi tức thì (không nội suy)
  currentFace.eyeShapeType = targetFace.eyeShapeType; 
  
  // Nội suy tuyến tính (Lerp) các tham số kích thước
  currentFace.eyeWidth    += (targetFace.eyeWidth    - currentFace.eyeWidth)    * lerpSpeed;
  currentFace.eyeHeight   += (targetFace.eyeHeight   - currentFace.eyeHeight)   * lerpSpeed;
  currentFace.eyeRadius   += (targetFace.eyeRadius   - currentFace.eyeRadius)   * lerpSpeed;
  currentFace.eyeAngle    += (targetFace.eyeAngle    - currentFace.eyeAngle)    * lerpSpeed;
  currentFace.glowSize    += (targetFace.glowSize    - currentFace.glowSize)    * lerpSpeed;
  currentFace.mouthHeight += (targetFace.mouthHeight - currentFace.mouthHeight) * lerpSpeed;
  currentFace.mouthWidth  += (targetFace.mouthWidth  - currentFace.mouthWidth)  * lerpSpeed;
  currentFace.mouthGlowSize += (targetFace.mouthGlowSize - currentFace.mouthGlowSize) * lerpSpeed;
}

// Khối vẽ 1 mắt (Tạo Sub-sprite -> Góc xoay -> Bóng viền -> Masking)
void drawEye(float centerX, float centerY, bool isRightEye) {
  eyeSprite.fillSprite(TFT_BLACK); // Xóa nền trong suốt
  
  float pivotX = 60; // Tâm X của Sub-Sprite 120x120
  float pivotY = 60; // Tâm Y của Sub-Sprite
  eyeSprite.setPivot(pivotX, pivotY);

  uint32_t faceColor = tft.color565(0, 255, 255); // Cyan
  uint32_t glowColor = tft.color565(0, 100, 100); // Dark Cyan (Nghệ thuật Visual Illusion)

  // 1. Vẽ Glow dưới cùng
  if (currentFace.glowSize > 0.5) {
    eyeSprite.fillRoundRect(
      pivotX - currentFace.eyeWidth/2 - currentFace.glowSize,
      pivotY - currentFace.eyeHeight/2 - currentFace.glowSize,
      currentFace.eyeWidth + currentFace.glowSize*2,
      currentFace.eyeHeight + currentFace.glowSize*2,
      currentFace.eyeRadius, glowColor);
  }

  // 2. Vẽ Mắt Chính
  eyeSprite.fillRoundRect(
    pivotX - currentFace.eyeWidth/2,
    pivotY - currentFace.eyeHeight/2,
    currentFace.eyeWidth,
    currentFace.eyeHeight,
    currentFace.eyeRadius, faceColor);

  // 3. Thủ thuật Masking để tạo kiểu dáng (ShapeType) mà không cần tính đa giác
  int type = (int)(currentFace.eyeShapeType + 0.5);
  if (type == 1) { // Bán nguyệt trên (Vui): Cắt 1/2 dưới
    eyeSprite.fillRect(0, pivotY, 120, 60, TFT_BLACK);
  } else if (type == 2) { // Bán nguyệt dưới (Buồn): Cắt 1/2 trên
    eyeSprite.fillRect(0, 0, 120, pivotY, TFT_BLACK);
  } else if (type == 3) { // Oval phẳng đáy (Ngạc nhiên): Cắt 1/4 dưới
    eyeSprite.fillRect(0, pivotY + currentFace.eyeHeight/4, 120, 60, TFT_BLACK);
  }

  // 4. Xoay và đè lên Canvas Chính (Loại bỏ màu đen -> Transparent)
  canvasSprite.setPivot(centerX, centerY);
  float angle = isRightEye ? -currentFace.eyeAngle : currentFace.eyeAngle;
  eyeSprite.pushRotated(&canvasSprite, angle, TFT_BLACK);
}

void renderToScreen() {
  // 1. Xóa nền Sprite tổng
  canvasSprite.fillSprite(tft.color565(0, 0, 0));

  // 2. Vẽ Hai Mắt bằng Sub-Sprite (Áp dụng xoay & che mask)
  drawEye(60, 90, false); // Mắt trái
  drawEye(180, 90, true); // Mắt phải

  // 3. Xử lý logic Miệng
  uint32_t faceColor = tft.color565(0, 255, 255);
  uint32_t glowColor = tft.color565(0, 100, 100);

  if (currentFace.mouthHeight > 0.5) {
    float mouthX = 120 - currentFace.mouthWidth/2;
    float mouthY = 125 - currentFace.mouthHeight/2;
    
    if (currentFace.mouthGlowSize > 0.5) {
      canvasSprite.fillRoundRect(
        mouthX - currentFace.mouthGlowSize,
        mouthY - currentFace.mouthGlowSize,
        currentFace.mouthWidth + currentFace.mouthGlowSize*2,
        currentFace.mouthHeight + currentFace.mouthGlowSize*2,
        6, glowColor);
    }
    canvasSprite.fillRoundRect(mouthX, mouthY, currentFace.mouthWidth, currentFace.mouthHeight, 6, faceColor);
  }

  // 4. Đẩy DMA
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
  
  tft.init();
  tft.setRotation(0); 
  tft.fillScreen(tft.color565(0, 0, 0)); 

  // Cấp phát PSRAM cho 2 lớp Sprite (Canvas & Eye)
  canvasSprite.setPsram(true); 
  canvasSprite.setColorDepth(16); 
  eyeSprite.setPsram(true);
  eyeSprite.setColorDepth(16);

  if (canvasSprite.createSprite(240, 240) == nullptr) {
    Serial.println("LỖI: Không thể tạo Canvas Sprite!");
    while (1); 
  }
  if (eyeSprite.createSprite(120, 120) == nullptr) {
    Serial.println("LỖI: Không thể tạo Eye Sprite!");
    while (1); 
  }
  
  Serial.println("Khoi tao Face Voi LovyanGFX thanh cong!");
}

void loop() {
  unsigned long now = millis();

  if (now - lastUpdate >= 50) {
    lastUpdate = now;
    updateFaceLogic();
    renderToScreen();
  }

  if (now - stateChangeTimer > 2500) {
    stateChangeTimer = now;
    currentStateIndex = (currentStateIndex + 1) % 4;
    
    switch(currentStateIndex) {
      case 0: targetFace = stateNormal; break;
      case 1: targetFace = stateSmile; break;
      case 2: targetFace = stateTalkOpen; break;
      case 3: targetFace = stateIdle; break;
    }
  }
}



