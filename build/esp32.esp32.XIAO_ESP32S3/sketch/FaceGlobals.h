#line 1 "C:\\rust\\face_rbot\\FaceGlobals.h"
#ifndef FACE_GLOBALS_H
#define FACE_GLOBALS_H

#include <Arduino.h>
#include "FaceTypes.h"

// Biến trạng thái hiển thị hiện tại
extern FaceState currentFace;
extern FaceState targetFace;

// Giao tiếp liên lõi (Inter-core Communication)
extern volatile int targetEmotionCode; 
extern int lastEmotionCode; 
extern volatile unsigned long lastInteractionTime; 

// Đồng hồ đếm giờ cho Animation
extern unsigned long winkStartTime;
extern unsigned long sleepStartTime;
extern bool winkDirection; 
extern int sleepBlinkCount;

// Trạng thái nháy đèn GPIO 1 (Báo hiệu đổi mặt)
extern unsigned long stateBlinkStartTime;
extern bool isStateBlinking;

// --- OVERRIDE: BLINK MANAGER ---
extern float blinkFactor;
extern float targetBlinkFactor;
extern unsigned long lastBlinkTime;
extern unsigned long nextBlinkDelay;

// --- OVERRIDE: ANIMATION WEIGHTS ---
extern float susWeight;
extern float smugWeight;
extern float lerpSpeed; 

#endif
