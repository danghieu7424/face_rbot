#line 1 "C:\\rust\\face_rbot\\Comms.h"
#ifndef COMMS_H
#define COMMS_H

#include <Arduino.h>
#include "FaceGlobals.h"

/****
 * Task chạy trên Core 0 (Độc lập với Vẽ đồ họa).
 * Chuyên nhận lệnh UART từ ESP khác (Master).
 ****/
void AITask(void *pvParameters);

#endif
