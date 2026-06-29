#line 1 "C:\\rust\\face_rbot\\FaceTypes.cpp"
#include "FaceTypes.h"

// Khớp 100% với file HTML của user, bổ sung offsetX, offsetY
const FaceState stateNormal    = {0, 40, 50, 20,  7, 4, 14,  7, 40, 2, 4,   0,   0};
const FaceState stateIdle      = {0, 40, 50, 20,  7, 4, 14,  0,  0, 0, 0,   0,   0};  
const FaceState stateHappy     = {1, 45, 25, 20, 12, 4, 14, 25, 55, 2, 4,   0,  -5}; 
const FaceState stateSad       = {0, 35, 40, 15,-10, 4, 14,  4, 20, 2, 4,   0,  15}; 
const FaceState stateLookLeft  = {0, 35, 50, 20,  5, 4, 14,  5, 25, 2, 4, -30,   0};
const FaceState stateLookRight = {0, 35, 50, 20,  5, 4, 14,  5, 25, 2, 4,  30,   0};
const FaceState stateTalk      = {0, 38, 55, 18,  7, 4, 14, 35, 30, 2, 4,   0,  -2};
const FaceState stateSleep     = {0, 40,  5,  3,  0, 4, 14,  0,  0, 0, 0,   0,  15}; // Mắt khép nhỏ, đầu cúi xuống (offsetY=15)
const FaceState stateAngry     = {0, 40, 25, 10, 25, 4, 14,  5, 40, 2, 4,   0,   5}; // Mắt dẹt, góc nghiêng gắt, miệng rộng và bẹt
const FaceState stateSurprised = {0, 50, 60, 25,  0, 4, 14, 40, 25, 2, 4,   0, -15}; // Mắt mở to tròn, miệng chữ O dài, đầu giật lên
const FaceState stateDoubt     = {0, 35, 15,  5,  0, 4, 14,  4, 15, 2, 4,  25,   5}; // Mắt híp (squint), liếc sang một bên nghi ngờ
const FaceState stateCry       = {0, 35, 20, 10,-15, 4, 14, 7, 20, 2, 4,   0,  10}; // Mắt nheo xuống, mếu máo
const FaceState stateDizzy     = {0, 50, 50, 20,  0, 4, 14, 20, 20, 2, 4,   0,   0}; // Hình dáng bình thường nhưng sẽ quay vòng vòng
const FaceState stateWink      = {0, 40, 50, 20,  7, 4, 14, 15, 40, 2, 4,   0,   0}; // Một mắt nhắm một mắt mở (xử lý logic riêng)
const FaceState statePanic     = {0, 65, 65, 32,  0, 4, 14,  5, 15, 2, 4,   0,   0}; // Mắt mở to tròn hết cỡ, miệng chữ O nhỏ
const FaceState stateSmug      = {1, 40, 20, 15, 10, 4, 14,  5, 25, 2, 4,  10, -5}; // Bán nguyệt trên, liếc ngước, miệng nhếch lệch
const FaceState stateScan      = {0, 50, 50, 20,  0, 4, 14,  2, 40, 2, 4,   0,   0}; // Mắt bình thường, bên trong có radar quét
const FaceState stateBored     = {3, 40, 25, 10,  0, 4, 14,  2, 45, 2, 4,   0,  25}; // Mắt nửa vời, cúi gập gục đầu, miệng ngang dài
const FaceState stateLove      = {0, 55, 55, 25,  0, 4, 14, 25, 40, 2, 4,   0,  -5}; // Mắt to tròn long lanh, miệng cười tươi
const FaceState stateGlitch    = {4, 40, 40, 10,  0, 4, 14, 10, 30, 2, 4,   0,   0}; // Mắt vuông, giật lag loạn xạ
const FaceState stateSus       = {0, 40, 35, 17,  0, 4, 14,  5, 20, 2, 4,   0,   0}; // Hình thái cơ bản, sẽ bị override nháy 1 mắt
const FaceState stateFurious   = {2, 45, 25, 10, 25, 4, 14,  5, 45, 2, 4,   0,  15}; // Rất tức giận, ngước mặt, mồm há to
const FaceState statePetting   = {1, 45,  8, 20,  10, 4, 14, 15, 30, 2, 4,   0, -10}; // Mắt nhắm tít vui vẻ, mồm tủm tỉm, ngước đầu hưởng thụ
