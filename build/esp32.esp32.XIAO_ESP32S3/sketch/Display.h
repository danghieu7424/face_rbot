#line 1 "C:\\rust\\face_rbot\\Display.h"
#ifndef DISPLAY_H
#define DISPLAY_H

#include <LovyanGFX.hpp>
#include "FaceGlobals.h"

// Khởi tạo thư viện LCD
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX(void);
};

// Khai báo extern để dùng chung trên toàn hệ thống
extern LGFX tft;
extern LGFX_Sprite canvasSprite;
extern LGFX_Sprite eyeSprite;

// Các hàm liên quan tới màu sắc và đồ hoạ lõi
uint32_t lerpColor(uint32_t from, uint32_t to, float t);

/****
 * Thuật toán Scanline Rasterization vẽ bo góc Elip bất đối xứng + Gradient Dọc siêu mượt.
 * Sử dụng cho cả vẽ Mắt (isMouth = false) và vẽ Miệng (isMouth = true).
 ****/
void drawGradientAsymmetricRect(LGFX_Sprite* spr, float cx, float cy, float w, float h, float shapeType, uint32_t colorTop, uint32_t colorBot, bool isMouth, float pitchFactor = 0.0f);

#endif
