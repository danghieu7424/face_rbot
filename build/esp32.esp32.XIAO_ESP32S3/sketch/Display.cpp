#line 1 "C:\\rust\\face_rbot\\Display.cpp"
#include "Display.h"

LGFX tft;
LGFX_Sprite canvasSprite(&tft);
LGFX_Sprite eyeSprite(&tft);

LGFX::LGFX(void) {
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

// Sử dụng trực tiếp mã màu chuẩn RGB888 để lerp, tránh lỗi Byte-swapping của màn hình LCD
uint32_t lerpColor(uint32_t from, uint32_t to, float t) {
  uint8_t r1 = (from >> 16) & 0xFF;
  uint8_t g1 = (from >> 8) & 0xFF;
  uint8_t b1 = from & 0xFF;

  uint8_t r2 = (to >> 16) & 0xFF;
  uint8_t g2 = (to >> 8) & 0xFF;
  uint8_t b2 = to & 0xFF;

  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; 
}

void drawGradientAsymmetricRect(LGFX_Sprite* spr, float cx, float cy, float w, float h, float shapeType, uint32_t colorTop, uint32_t colorBot, bool isMouth, float pitchFactor) {
  float rTL_x=0, rTL_y=0, rTR_x=0, rTR_y=0, rBR_x=0, rBR_y=0, rBL_x=0, rBL_y=0;
  
  if (!isMouth) {
    int type = (int)(shapeType + 0.5);
    
    // Tính tỉ lệ nhắm mắt hiện tại
    float blinkRatio = h / (currentFace.eyeHeight > 0.1f ? currentFace.eyeHeight : 40.0f);
    if (blinkRatio > 1.0f) blinkRatio = 1.0f;
    
    // Thu nhỏ bán kính (Radius) cực nhanh khi nhắm mắt để tạo hiệu ứng mí mắt khép dẹt thành đường thẳng
    float r = currentFace.eyeRadius * blinkRatio * blinkRatio; 
    if (r < 1.0f) r = 1.0f; // Vẫn giữ 1 pixel bo nhẹ để không bị gắt
    
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
    // Miệng: Góc trên bo từ 0% (phẳng) lên 50% (tròn) khi miệng mở lớn
    float topRadiusFactor = shapeType; 
    float bottomRadiusFactor = 1.0f - topRadiusFactor;

    rTL_x = rTR_x = w * topRadiusFactor; rTL_y = rTR_y = h * topRadiusFactor;
    rBL_x = rBR_x = w * 0.5f;  rBL_y = rBR_y = h * bottomRadiusFactor;
  }

  // Chống tràn bán kính
  if (rTL_x > w/2) rTL_x = w/2; if (rTR_x > w/2) rTR_x = w/2;
  if (rBL_x > w/2) rBL_x = w/2; if (rBR_x > w/2) rBR_x = w/2;
  if (rTL_y > h/2) rTL_y = h/2; if (rTR_y > h/2) rTR_y = h/2;

  if (!isMouth) {
    if (rBL_y > h/2) rBL_y = h/2; if (rBR_y > h/2) rBR_y = h/2; 
  } else {
    if (rBL_y > h) rBL_y = h; if (rBR_y > h) rBR_y = h; 
  }

  int startX = cx - w/2;
  int startY = cy - h/2;
  int H = (int)h;
  int W = (int)w;

  for (int y = 0; y < H; y++) {
    float x_start = 0;
    float x_end = W - 1;

    // Cộng 0.5f để căn tâm Pixel, làm mượt viền
    if (rTL_y > 0 && y < rTL_y) {
      float dy = rTL_y - y - 1.0f; 
      float val = 1.0f - (dy * dy) / (rTL_y * rTL_y);
      if (val < 0) val = 0;
      x_start = rTL_x - (rTL_x * sqrt(val));
    }
    if (rTR_y > 0 && y < rTR_y) {
      float dy = rTR_y - y - 1.0f;
      float val = 1.0f - (dy * dy) / (rTR_y * rTR_y);
      if (val < 0) val = 0;
      x_end = W - 1 - (rTR_x - (rTR_x * sqrt(val)));
    }
    if (rBL_y > 0 && y >= H - rBL_y) {
      float dy = y - (H - rBL_y) + 1.0f;
      float val = 1.0f - (dy * dy) / (rBL_y * rBL_y);
      if (val < 0) val = 0;
      x_start = rBL_x - (rBL_x * sqrt(val));
    }
    if (rBR_y > 0 && y >= H - rBR_y) {
      float dy = y - (H - rBR_y) + 1.0f;
      float val = 1.0f - (dy * dy) / (rBR_y * rBR_y);
      if (val < 0) val = 0;
      x_end = W - 1 - (rBR_x - (rBR_x * sqrt(val)));
    }

    // --- HIỆU ỨNG PARALLAX 3D PITCH (TRAPEZOID) ---
    float depthScale = 1.0f + pitchFactor * (((float)y / H) - 0.5f);
    float center = W / 2.0f;
    x_start = center + (x_start - center) * depthScale;
    x_end = center + (x_end - center) * depthScale;

    float t = (H > 1) ? (float)y / (H - 1) : 0;
    uint32_t color = lerpColor(colorTop, colorBot, t);

    if (x_end >= x_start) {
      spr->drawFastHLine(startX + (int)x_start, startY + y, (int)(x_end - x_start + 1), color);
    }
  }
}
