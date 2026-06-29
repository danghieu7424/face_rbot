#line 1 "C:\\rust\\face_rbot\\Comms.cpp"
#include "Comms.h"
#include "AI_Logic.h"
void AITask(void *pvParameters) {
  Serial.println("=========================================");
  Serial.println("AI DANG DUOC TAM DUNG DE DEBUG.");
  Serial.println("Vui long nhap so tu 0 den 20 qua UART:");
  Serial.println("0:Idle 1:Normal 2:Happy 3:Sad 4:Talk 5:Sleep");
  Serial.println("6:Angry 7:Surprised 8:Doubt 9:Cry 10:Dizzy");
  Serial.println("11:Wink 12:LookAround 13:Panic 14:Smug");
  Serial.println("15:Scan 16:Bored 17:Love 18:Glitch 19:Sus 20:Furious 21:Petting");
  Serial.println("=========================================");

  for (;;) {
    // 1. Kiểm tra Serial Input (Giữ nguyên)
    if (Serial.available() > 0) {
      int code = Serial.parseInt();
      while(Serial.available() > 0) Serial.read(); // Đọc bỏ ký tự thừa

      if (code >= 0 && code < NUM_ACTIONS) {
        targetEmotionCode = code;
        lastInteractionTime = millis(); // Reset thời gian rảnh
        Serial.print(">> [SERIAL] Chuyen sang trang thai: ");
        Serial.println(code);
      } else {
        Serial.println("Loi: Ma cam xuc phai tu 0 den 20.");
      }
    }

    // Quét Serial mỗi 100ms
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}
