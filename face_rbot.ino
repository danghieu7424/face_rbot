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

// Thuật toán Scanline Rasterization vẽ bo góc Elip bất đối xứng + Gradient Dọc (VGradient) siêu mượt
void drawGradientAsymmetricRect(LGFX_Sprite* spr, float cx, float cy, float w, float h, float shapeType, uint32_t colorTop, uint32_t colorBot, bool isMouth) {
  float rTL_x=0, rTL_y=0, rTR_x=0, rTR_y=0, rBR_x=0, rBR_y=0, rBL_x=0, rBL_y=0;
  
  if (!isMouth) {
    int type = (int)(shapeType + 0.5);
    float r = currentFace.eyeRadius;
    if (type == 0) { 
      rTL_x = rTL_y = rTR_x = rTR_y = rBR_x = rBR_y = rBL_x = rBL_y = r;
    } else if (type == 1) { // Bán nguyệt trên
      rTL_x = rTR_x = w/2; rTL_y = rTR_y = h/2;
      rBL_x = rBR_x = w*0.1; rBL_y = rBR_y = h*0.1;
    } else if (type == 2) { // Bán nguyệt dưới
      rTL_x = rTR_x = w*0.1; rTL_y = rTR_y = h*0.1;
      rBL_x = rBR_x = w/2; rBL_y = rBR_y = h/2;
    } else if (type == 3) { // Oval phẳng đáy
      rTL_x = rTR_x = w/2; rTL_y = rTR_y = h/2;
      rBL_x = rBR_x = w*0.2; rBL_y = rBR_y = h*0.2;
    }
  } else {
    // Miệng: 15% 15% 50% 50% / 15% 15% 100% 100%
    rTL_x = rTR_x = w * 0.15; rTL_y = rTR_y = h * 0.15;
    rBL_x = rBR_x = w * 0.5;  rBL_y = rBR_y = h * 1.0;
  }

  // Chống tràn bán kính
  if (rTL_x > w/2) rTL_x = w/2; if (rTR_x > w/2) rTR_x = w/2;
  if (rBL_x > w/2) rBL_x = w/2; if (rBR_x > w/2) rBR_x = w/2;
  if (rTL_y > h/2) rTL_y = h/2; if (rTR_y > h/2) rTR_y = h/2;
  if (rBL_y > h) rBL_y = h; if (rBR_y > h) rBR_y = h; 

  int startX = cx - w/2;
  int startY = cy - h/2;
  int H = (int)h;
  int W = (int)w;

  for (int y = 0; y < H; y++) {
    float x_start = 0;
    float x_end = W - 1;

    // Cộng 0.5f để căn tâm Pixel, làm mượt viền
    if (rTL_y > 0 && y < rTL_y) {
      float dy = rTL_y - y - 0.5f; 
      float val = 1.0f - (dy * dy) / (rTL_y * rTL_y);
      if (val < 0) val = 0;
      x_start = rTL_x - (rTL_x * sqrt(val));
    }
    if (rTR_y > 0 && y < rTR_y) {
      float dy = rTR_y - y - 0.5f;
      float val = 1.0f - (dy * dy) / (rTR_y * rTR_y);
      if (val < 0) val = 0;
      x_end = W - 1 - (rTR_x - (rTR_x * sqrt(val)));
    }
    if (rBL_y > 0 && y >= H - rBL_y) {
      float dy = y - (H - rBL_y) + 0.5f;
      float val = 1.0f - (dy * dy) / (rBL_y * rBL_y);
      if (val < 0) val = 0;
      x_start = rBL_x - (rBL_x * sqrt(val));
    }
    if (rBR_y > 0 && y >= H - rBR_y) {
      float dy = y - (H - rBR_y) + 0.5f;
      float val = 1.0f - (dy * dy) / (rBR_y * rBR_y);
      if (val < 0) val = 0;
      x_end = W - 1 - (rBR_x - (rBR_x * sqrt(val)));
    }

    float t = (H > 1) ? (float)y / (H - 1) : 0;
    uint32_t color = lerpColor(colorTop, colorBot, t);

    if (x_end >= x_start) {
      spr->drawFastHLine(startX + (int)x_start, startY + y, (int)(x_end - x_start + 1), color);
    }
  }
}

void drawEye(float centerX, float centerY, bool isRightEye) {
  eyeSprite.fillSprite(TFT_BLACK);
  float pivotX = 60, pivotY = 60;
  eyeSprite.setPivot(pivotX, pivotY);

  uint32_t colorTop = tft.color565(0, 255, 255); // Cyan (Màu lõi sáng)
  // ĐÃ LOẠI BỎ colorBot ĐỂ TRÁNH LỖI MOIRÉ KHI XOAY
  uint32_t shadowColor = tft.color565(0, 20, 50); // Dark Blue (Màu nền bóng)

  float w = currentFace.eyeWidth;
  float h = currentFace.eyeHeight;
  float shape = currentFace.eyeShapeType;

  // 1. Vẽ Bóng Giả (Dark Blue) làm nền nguyên khối
  drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY, w, h, shape, shadowColor, shadowColor, false);

  // 2. Vẽ Lõi Sáng (ÉP CHẠY MÀU SOLID BẰNG CÁCH TRUYỀN colorTop CHO CẢ 2 THAM SỐ)
  // Sự đồng nhất màu sẽ khiến hiện tượng xé dòng (banding) khi xoay biến mất hoàn toàn
  drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 2, w - 2, h - 2, shape, colorTop, colorTop, false);

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
    // ĐÃ LOẠI BỎ colorBot CỦA MIỆNG
    uint32_t shadowColor = tft.color565(0, 20, 50);

    // Bóng giả cho Miệng
    drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY, w, h, 0, shadowColor, shadowColor, true);
    
    // Lõi sáng dịch lên (ÉP CHẠY MÀU SOLID TƯƠNG TỰ MẮT ĐỂ ĐỒNG BỘ)
    drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY - 2, w - 2, h - 2, 0, colorTop, colorTop, true);
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



