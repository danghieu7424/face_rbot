#line 1 "C:\\rust\\face_rbot\\FaceTypes.h"
#ifndef FACE_TYPES_H
#define FACE_TYPES_H

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
  float offsetX;
  float offsetY;
};

// Khai báo extern để file nào include FaceTypes.h cũng thấy được các biến const này
extern const FaceState stateNormal;
extern const FaceState stateIdle;
extern const FaceState stateHappy;
extern const FaceState stateSad;
extern const FaceState stateLookLeft;
extern const FaceState stateLookRight;
extern const FaceState stateTalk;
extern const FaceState stateSleep;
extern const FaceState stateAngry;
extern const FaceState stateSurprised;
extern const FaceState stateDoubt;
extern const FaceState stateCry;
extern const FaceState stateDizzy;
extern const FaceState stateWink;
extern const FaceState statePanic;
extern const FaceState stateSmug;
extern const FaceState stateScan;
extern const FaceState stateBored;
extern const FaceState stateLove;
extern const FaceState stateGlitch;
extern const FaceState stateSus;
extern const FaceState stateFurious;
extern const FaceState statePetting;

#endif
