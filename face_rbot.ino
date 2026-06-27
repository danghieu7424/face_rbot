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

uint32_t lerpColor(uint32_t c1, uint32_t c2, float t) {
  float r1 = (c1 >> 11) & 0x1F;
  float g1 = (c1 >> 5)  & 0x3F;
  float b1 = c1         & 0x1F;

  float r2 = (c2 >> 11) & 0x1F;
  float g2 = (c2 >> 5)  & 0x3F;
  float b2 = c2         & 0x1F;

  uint16_t r = (uint16_t)(r1 + (r2 - r1) * t + 0.5f);
  uint16_t g = (uint16_t)(g1 + (g2 - g1) * t + 0.5f);
  uint16_t b = (uint16_t)(b1 + (b2 - b1) * t + 0.5f);

  return (r << 11) | (g << 5) | b;
}

#define MASK_CORE   0xFFFE
#define MASK_SHADOW 0xFFFD

void applyScreenSpaceDeferredShading(uint32_t colorTop, uint32_t colorBot, uint32_t shadowColor) {
  uint16_t* buffer = (uint16_t*)canvasSprite.getBuffer();
  int width = 240;
  uint16_t shadow16 = (uint16_t)shadowColor;

  for (int y = 0; y < 240; y++) {
    // VGradient gốc màn hình (Mắt robot thường nằm từ tọa độ Y=40 -> 140)
    float t = (y - 40.0f) / 100.0f;
    if (t < 0) t = 0; 
    if (t > 1) t = 1;
    uint32_t c32 = lerpColor(colorTop, colorBot, t);
    uint16_t grad16 = (uint16_t)c32;

    for (int x = 0; x < width; x++) {
      int index = y * width + x;
      if (buffer[index] == MASK_CORE) {
        buffer[index] = grad16;
      } else if (buffer[index] == MASK_SHADOW) {
        buffer[index] = shadow16;
      }
    }
  }
}

void drawEye(float centerX, float centerY, bool isRightEye) {
  eyeSprite.fillSprite(TFT_BLACK);
  float pivotX = 60, pivotY = 60;
  eyeSprite.setPivot(pivotX, pivotY);

  float w = currentFace.eyeWidth;
  float h = currentFace.eyeHeight;
  float r = currentFace.eyeRadius;

  // Vẽ khối MASK tĩnh
  eyeSprite.fillRoundRect(pivotX - w/2, pivotY - h/2, w, h, r, MASK_SHADOW);
  eyeSprite.fillRoundRect(pivotX - w/2 + 2, pivotY - h/2 - 2, w - 4, h - 4, r - 1, MASK_CORE);

  // Cắt hình (Masking theo Shape Type)
  int type = (int)(currentFace.eyeShapeType + 0.5);
  if (type == 1) { 
    eyeSprite.fillRect(0, pivotY, 120, 60, TFT_BLACK);
  } else if (type == 2) { 
    eyeSprite.fillRect(0, 0, 120, pivotY, TFT_BLACK);
  } else if (type == 3) { 
    eyeSprite.fillRect(0, pivotY + h/4, 120, 60, TFT_BLACK);
  }

  // Đẩy khối MASK ra Screen với góc xoay (sẽ có răng cưa ở viền quay, nhưng KHÔNG CÓ RĂNG CƯA GRADIENT)
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
    
    // Miệng cũng sử dụng MASK
    canvasSprite.fillRoundRect(mouthX - w/2, mouthY - h/2, w, h, h/2, MASK_SHADOW);
    canvasSprite.fillRoundRect(mouthX - w/2 + 2, mouthY - h/2 - 2, w - 4, h - 4, (h-4)/2, MASK_CORE);
  }

  uint32_t colorTop = tft.color565(0, 255, 255);
  uint32_t colorBot = tft.color565(0, 0, 255);
  uint32_t shadowColor = tft.color565(0, 0, 50);

  // Kích hoạt Engine Deferred Shading để tô màu Gradient hoàn hảo lên Screen!
  applyScreenSpaceDeferredShading(colorTop, colorBot, shadowColor);

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



