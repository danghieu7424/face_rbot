#line 1 "C:\\rust\\face_rbot\\Animation.h"
#ifndef ANIMATION_H
#define ANIMATION_H

#include <Arduino.h>
#include "FaceGlobals.h"
#include "Display.h"

/****
 * Hàm tính toán logic chuyển tiếp (Lerp) của khuôn mặt.
 * Điều chỉnh độ mở mắt, tốc độ nháy, biên độ miệng tùy theo trạng thái cảm xúc (targetEmotionCode).
 ****/
void updateFaceLogic();

/****
 * Hàm vẽ mắt trái/phải lên Sprite.
 ****/
void drawEye(float centerX, float centerY, bool isRightEye, float scale3D = 1.0f, float extraAngle = 0.0f, float customBlink = -1.0f, float pitchFactor = 0.0f);

/****
 * Hàm dựng hình chính.
 * Tổng hợp các Sprite (mắt, miệng) và vẽ ra màn hình thực tế. Bao gồm các hiệu ứng hoặt họa động (VFX).
 ****/
void renderToScreen();

#endif
