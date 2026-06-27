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

// Hàm vẽ Hình học Bất đối xứng (Primitive CSS border-radius algorithm)
void drawAsymmetricRect(LGFX_Sprite* spr, float x, float y, float w, float h, float shapeType, uint32_t color, bool isMouth) {
  float rTL_x=0, rTL_y=0, rTR_x=0, rTR_y=0, rBR_x=0, rBR_y=0, rBL_x=0, rBL_y=0;
  
  if (!isMouth) {
    int type = (int)(shapeType + 0.5);
    float r = currentFace.eyeRadius;
    if (type == 0) { // Oval chuẩn
      rTL_x = rTL_y = rTR_x = rTR_y = rBR_x = rBR_y = rBL_x = rBL_y = r;
    } else if (type == 1) { // Bán nguyệt trên (50% 50% 10% 10% / 100% 100% 10% 10%)
      rTL_x = rTR_x = w/2; rTL_y = rTR_y = h/2;
      rBL_x = rBR_x = w*0.1; rBL_y = rBR_y = h*0.1;
    } else if (type == 2) { // Bán nguyệt dưới
      rTL_x = rTR_x = w*0.1; rTL_y = rTR_y = h*0.1;
      rBL_x = rBR_x = w/2; rBL_y = rBR_y = h/2;
    } else if (type == 3) { // Oval đáy phẳng
      rTL_x = rTR_x = w/2; rTL_y = rTR_y = h/2;
      rBL_x = rBR_x = w*0.2; rBL_y = rBR_y = h*0.2;
    }
  } else {
    // CSS Miệng: 15% 15% 50% 50% / 15% 15% 100% 100%
    rTL_x = rTR_x = w * 0.15; rTL_y = rTR_y = h * 0.15;
    rBL_x = rBR_x = w * 0.5;  rBL_y = rBR_y = h * 1.0;
  }

  // Chống tràn bán kính (Limit Radius)
  if (rTL_x > w/2) rTL_x = w/2; if (rTR_x > w/2) rTR_x = w/2;
  if (rBL_x > w/2) rBL_x = w/2; if (rBR_x > w/2) rBR_x = w/2;
  if (rTL_y > h/2) rTL_y = h/2; if (rTR_y > h/2) rTR_y = h/2;
  if (rBL_y > h) rBL_y = h; if (rBR_y > h) rBR_y = h; 

  spr->setClipRect(x, y, w, h);

  // Vẽ 4 góc bằng hình Elip
  if(rTL_x > 0 && rTL_y > 0) spr->fillEllipse(x + rTL_x, y + rTL_y, rTL_x, rTL_y, color);
  if(rTR_x > 0 && rTR_y > 0) spr->fillEllipse(x + w - rTR_x, y + rTR_y, rTR_x, rTR_y, color);
  if(rBR_x > 0 && rBR_y > 0) spr->fillEllipse(x + w - rBR_x, y + h - rBR_y, rBR_x, rBR_y, color);
  if(rBL_x > 0 && rBL_y > 0) spr->fillEllipse(x + rBL_x, y + h - rBL_y, rBL_x, rBL_y, color);

  // Vẽ 2 khối chữ nhật chữ thập để nối các góc
  spr->fillRect(x + rTL_x, y, w - rTL_x - rTR_x, h, color);
  float maxYTop = (rTL_y > rTR_y) ? rTL_y : rTR_y;
  float maxYBot = (rBL_y > rBR_y) ? rBL_y : rBR_y;
  spr->fillRect(x, y + maxYTop, w, h - maxYTop - maxYBot + 1, color);

  spr->clearClipRect();
}

// Hàm xử lý Alpha Blending Glow và Inner Shadow CSS
void drawAdvancedGlowShadow(LGFX_Sprite* spr, float cx, float cy, float w, float h, float shapeType, float glow, float innerShadow, uint32_t faceColor, bool isMouth) {
  uint32_t glowBase = tft.color565(0, 100, 100);
  
  // 1. Vẽ Glow (box-shadow) bằng Alpha Blending (Vẽ đa giác từ to đến nhỏ)
  int glowSteps = (int)glow;
  if (glowSteps > 0) {
    for (int i = glowSteps; i >= 1; i--) {
      float t = (float)i / glowSteps; 
      uint32_t c = lerpColor(TFT_BLACK, glowBase, t); // Fade to Black
      drawAsymmetricRect(spr, cx - w/2 - i, cy - h/2 - i, w + i*2, h + i*2, shapeType, c, isMouth);
    }
  }

  // 2. Vẽ Inner Shadow (inset box-shadow offset -Y)
  uint32_t shadowColor = tft.color565(0, 50, 50); // Màu đáy tối
  drawAsymmetricRect(spr, cx - w/2, cy - h/2, w, h, shapeType, shadowColor, isMouth);

  int shadowSteps = (int)innerShadow;
  if (shadowSteps > 0) {
    for (int i = 1; i <= shadowSteps; i++) {
      float t = (float)i / shadowSteps; // Sáng dần lên faceColor
      uint32_t c = lerpColor(shadowColor, faceColor, t);
      // Dịch đa giác lên trên i pixel để tạo khoảng tối ở đáy (Inset Y = -innerShadow)
      float shrink = i * 0.5; // Thu nhỏ nhẹ để không tràn hai bên viền
      drawAsymmetricRect(spr, cx - w/2 + shrink, cy - h/2 - i + shrink/2, w - shrink*2, h - i, shapeType, c, isMouth);
    }
  } else {
    // Không có shadow thì vẽ luôn màu chính đè lên
    drawAsymmetricRect(spr, cx - w/2, cy - h/2, w, h, shapeType, faceColor, isMouth);
  }
}

void drawEye(float centerX, float centerY, bool isRightEye) {
  eyeSprite.fillSprite(TFT_BLACK);
  float pivotX = 60, pivotY = 60;
  eyeSprite.setPivot(pivotX, pivotY);

  drawAdvancedGlowShadow(&eyeSprite, pivotX, pivotY, currentFace.eyeWidth, currentFace.eyeHeight, currentFace.eyeShapeType, currentFace.glowSize, currentFace.innerShadow, tft.color565(0, 255, 255), false);

  canvasSprite.setPivot(centerX, centerY);
  float angle = isRightEye ? -currentFace.eyeAngle : currentFace.eyeAngle;
  eyeSprite.pushRotated(&canvasSprite, angle, TFT_BLACK); // Xoay lượng giác Sub-Sprite
}

void renderToScreen() {
  canvasSprite.fillSprite(tft.color565(0, 0, 0));

  drawEye(60, 90, false); // Trái
  drawEye(180, 90, true); // Phải

  if (currentFace.mouthHeight > 0.5) {
    float mouthX = 120;
    float mouthY = 125;
    drawAdvancedGlowShadow(&canvasSprite, mouthX, mouthY, currentFace.mouthWidth, currentFace.mouthHeight, 0, currentFace.mouthGlowSize, currentFace.mouthInnerShadow, tft.color565(0, 255, 255), true);
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



