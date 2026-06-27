#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX(void) {
    auto cfg = _bus_instance.config();
    cfg.spi_host = SPI2_HOST;  
    cfg.spi_mode = 3;          
    cfg.freq_write = 20000000; 
    cfg.spi_3wire = false;     
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO; 

    cfg.pin_sclk = 10; 
    cfg.pin_mosi = 11; 
    cfg.pin_miso = -1; 
    cfg.pin_dc   = 13;  
    
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    auto p_cfg = _panel_instance.config();
    p_cfg.pin_cs   = -1;   
    p_cfg.pin_rst  = 12;   
    p_cfg.pin_busy = -1;
    
    p_cfg.panel_width  = 240;
    p_cfg.panel_height = 240;
    p_cfg.offset_x = 0;    
    p_cfg.offset_y = 0;    
    p_cfg.invert = true;   
    p_cfg.rgb_order = false; 

    _panel_instance.config(p_cfg);
    setPanel(&_panel_instance);
  }
};

LGFX tft;
LGFX_Sprite canvasSprite(&tft);
LGFX_Sprite eyeSprite(&tft);

struct FaceState {
  float eyeShapeType; 
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

// Khớp 100% với file HTML của user
const FaceState stateNormal   = {0, 40, 50, 20, 7,  4, 14, 8,  40, 2, 4};
const FaceState stateSmile    = {1, 45, 20, 20, 10, 4, 14, 25, 60, 2, 4}; 
const FaceState stateIdle     = {0, 40, 50, 20, 7,  4, 14, 0,  0,  0, 0};  
const FaceState stateTalkOpen = {0, 38, 62, 10, 5,  4, 14, 35, 35, 2, 4};
const FaceState stateTalkClose= {0, 38, 62, 10, 5,  4, 14, 4,  35, 2, 4};

FaceState currentFace = stateNormal;
FaceState targetFace = stateIdle;

float lerpSpeed = 0.3; 

void updateFaceLogic() {
  currentFace.eyeShapeType = targetFace.eyeShapeType; 
  currentFace.eyeWidth    += (targetFace.eyeWidth    - currentFace.eyeWidth)    * lerpSpeed;
  currentFace.eyeHeight   += (targetFace.eyeHeight   - currentFace.eyeHeight)   * lerpSpeed;
  currentFace.eyeRadius   += (targetFace.eyeRadius   - currentFace.eyeRadius)   * lerpSpeed;
  currentFace.eyeAngle    += (targetFace.eyeAngle    - currentFace.eyeAngle)    * lerpSpeed;
  currentFace.glowSize    += (targetFace.glowSize    - currentFace.glowSize)    * lerpSpeed;
  currentFace.innerShadow += (targetFace.innerShadow - currentFace.innerShadow) * lerpSpeed;
  currentFace.mouthHeight += (targetFace.mouthHeight - currentFace.mouthHeight) * lerpSpeed;
  currentFace.mouthWidth  += (targetFace.mouthWidth  - currentFace.mouthWidth)  * lerpSpeed;
  currentFace.mouthGlowSize += (targetFace.mouthGlowSize - currentFace.mouthGlowSize) * lerpSpeed;
  currentFace.mouthInnerShadow += (targetFace.mouthInnerShadow - currentFace.mouthInnerShadow) * lerpSpeed;
}

uint32_t lerpColor(uint32_t from, uint32_t to, float t) {
  uint8_t r1 = (from >> 11) & 0x1F; r1 = (r1 << 3) | (r1 >> 2);
  uint8_t g1 = (from >> 5) & 0x3F;  g1 = (g1 << 2) | (g1 >> 4);
  uint8_t b1 = from & 0x1F;         b1 = (b1 << 3) | (b1 >> 2);

  uint8_t r2 = (to >> 11) & 0x1F; r2 = (r2 << 3) | (r2 >> 2);
  uint8_t g2 = (to >> 5) & 0x3F;  g2 = (g2 << 2) | (g2 >> 4);
  uint8_t b2 = to & 0x1F;         b2 = (b2 << 3) | (b2 >> 2);

  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;

  return tft.color565(r, g, b);
}

void drawEye(float centerX, float centerY, bool isRightEye) {
  eyeSprite.fillSprite(TFT_BLACK);
  float pivotX = 60, pivotY = 60;
  eyeSprite.setPivot(pivotX, pivotY);

  uint32_t colorTop = tft.color565(0, 255, 255); // Cyan (Màu lõi sáng)
  uint32_t colorBot = tft.color565(0, 50, 150);  // Dark Blue (Bóng tối giả)

  float w = currentFace.eyeWidth;
  float h = currentFace.eyeHeight;
  float r = currentFace.eyeRadius;

  // 1. Vẽ Bóng Giả (Dark Blue) làm nền (Fake Inner Shadow)
  eyeSprite.fillRoundRect(pivotX - w/2, pivotY - h/2, w, h, r, colorBot);

  // 2. Vẽ Lõi Sáng (Cyan) nhích lên trên một chút để hở viền tối ở dưới đáy
  // Nhích lên Y = -4, và thu nhỏ chiều ngang X một chút để tạo viền mượt
  eyeSprite.fillRoundRect(pivotX - w/2 + 2, pivotY - h/2 - 2, w - 4, h - 4, r - 1, colorTop);

  // 3. Masking: Che lấp phần thừa để tạo các Shape bất đối xứng (Siêu mượt, không vạch ngang)
  int type = (int)(currentFace.eyeShapeType + 0.5);
  if (type == 1) { // Bán nguyệt trên (Vui): Che nửa dưới
    eyeSprite.fillRect(0, pivotY, 120, 60, TFT_BLACK);
  } else if (type == 2) { // Bán nguyệt dưới (Buồn): Che nửa trên
    eyeSprite.fillRect(0, 0, 120, pivotY, TFT_BLACK);
  } else if (type == 3) { // Oval phẳng đáy (Ngạc nhiên): Che 1/4 dưới
    eyeSprite.fillRect(0, pivotY + h/4, 120, 60, TFT_BLACK);
  }

  // Xoay và in ra màn hình
  canvasSprite.setPivot(centerX, centerY);
  float angle = isRightEye ? -currentFace.eyeAngle : currentFace.eyeAngle;
  eyeSprite.pushRotated(&canvasSprite, angle, TFT_BLACK); 
}

void renderToScreen() {
  canvasSprite.fillSprite(tft.color565(0, 0, 0));

  drawEye(60, 90, false); 
  drawEye(180, 90, true); 

  if (currentFace.mouthHeight > 0.5) {
    float mouthX = 120;
    float mouthY = 125;
    float w = currentFace.mouthWidth;
    float h = currentFace.mouthHeight;
    
    uint32_t colorTop = tft.color565(0, 255, 255);
    uint32_t colorBot = tft.color565(0, 50, 150);

    // Bóng giả cho Miệng
    canvasSprite.fillRoundRect(mouthX - w/2, mouthY - h/2, w, h, h/2, colorBot);
    // Lõi sáng dịch lên
    canvasSprite.fillRoundRect(mouthX - w/2 + 2, mouthY - h/2 - 2, w - 4, h - 4, (h-4)/2, colorTop);
    
    // Masking để tạo hình nửa quả trứng (Chỉ giữ nửa dưới) cho Miệng nếu cần
    // Tạm thời dùng Oval bo tròn hoàn hảo cho miệng (r = h/2) 
  }

  canvasSprite.pushSprite(0, 0);
}

unsigned long lastUpdate = 0;
unsigned long stateChangeTimer = 0;
int currentStateIndex = 0;

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(0); 
  tft.fillScreen(tft.color565(0, 0, 0)); 

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



