#include <Arduino.h>
#include "FaceTypes.h"
#include "FaceGlobals.h"
#include "Display.h"
#include "Animation.h"
#include "Comms.h"
#include "AI_Logic.h"

void setup() {
  Serial.begin(115200);
  
  // LED GPIO 1 dùng để nháy khi có lệnh UART mới
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW);

  // Khởi tạo LCD
  tft.init();
  tft.setRotation(0); 

  canvasSprite.setColorDepth(16);
  canvasSprite.createSprite(240, 240);

  eyeSprite.setColorDepth(16);
  eyeSprite.createSprite(120, 120);

  // Tách biệt luồng nhận dữ liệu UART (Core 0) và đồ hoạ (Core 1)
  xTaskCreatePinnedToCore(
    AITask,       // Hàm thực thi
    "AI_Task",    // Tên task
    4096,         // Kích thước Stack
    NULL,         // Tham số
    1,            // Độ ưu tiên
    NULL,         // Task handle
    0             // Ghim vào Core 0
  );
}

void loop() {
  // --- HIỂN THỊ VÀ LOGIC MÀN HÌNH ---
  // AITask (Core 0) sẽ liên tục cập nhật biến targetEmotionCode từ UART.
  // Vòng lặp này (Core 1) chỉ tập trung vào việc vẽ ra màn hình.

  // 1. Logic Timeout Nhàn rỗi: Quá 15 giây không có lệnh UART -> Ngó nghiêng (12)
  if (targetEmotionCode == 0 || targetEmotionCode == 1) {
    if (millis() - lastInteractionTime > 15000) { 
      targetEmotionCode = 12; // Chuyển sang LookAround
      Serial.println(">> [AUTO] Khong co tuong tac 15s -> Tu dong chuyen sang nhin xung quanh (12)");
    }
  }

  // 2. Logic Timeout Ngủ gật: Đang ngó nghiêng (12) quá 10 phút (600,000ms) -> Đi ngủ (5)
  if (targetEmotionCode == 12) {
    if (millis() - lastInteractionTime > 600000) { 
      targetEmotionCode = 5; // Chuyển sang Sleep
      Serial.println(">> [AUTO] Khong co tuong tac 10 phut -> Tu dong di ngu (5)");
    }
  }

  // 4. Nhận biết sự thay đổi cảm xúc từ AI Task
  if (targetEmotionCode != lastEmotionCode) {
    // Nháy đèn GPIO 1 để báo hiệu nhận lệnh
    digitalWrite(1, HIGH);
    stateBlinkStartTime = millis();
    isStateBlinking = true;

    if (targetEmotionCode == 11) {
      winkStartTime = millis(); // Reset đồng hồ đo Wink
      winkDirection = !winkDirection; // Đảo hướng nháy mắt luân phiên
    }
    if (targetEmotionCode == 5) {
      sleepStartTime = millis(); // Reset đồng hồ đo Sleep
      sleepBlinkCount = 1; // Nháy 1 cái thật chậm, nặng nề
      nextBlinkDelay = 100; // Bắt đầu nháy ngay sau 100ms
      lastBlinkTime = millis();
    }
    lastEmotionCode = targetEmotionCode;
  }

  // Đọc lệnh cảm xúc từ AI Task
  switch (targetEmotionCode) {
    case 0: targetFace = stateIdle; break;
    case 1: targetFace = stateNormal; break;
    case 2: targetFace = stateHappy; break;
    case 3: targetFace = stateSad; break;
    case 4: targetFace = stateTalk; break;
    case 5: {
      // Chuỗi hiệu ứng Sleep: Chớp mắt mệt mỏi -> Ngáp dài -> Nhắm mắt gục ngủ
      unsigned long elapsed = millis() - sleepStartTime;
      if (elapsed > 5000) {
        targetFace = stateSleep; // Gục hẳn
      } else if (elapsed > 3500) {
        // Giai đoạn ngáp xong (3500ms - 5000ms): Nhắm nghiền mắt lại, gật gù và chép miệng "mlem mlem"
        targetFace = stateNormal; 
        targetFace.eyeAngle = -8;    // Đuôi mắt cụp hẳn xuống vì quá buồn ngủ
        
        // Mắt sụp xuống TỪ TỪ trong suốt 1.5s mấp máy môi (từ độ cao 25 xuống 5) - Cố gắng gượng ngủ
        float progress = (elapsed - 3500) / 1500.0; // Tính % hoàn thành của giai đoạn này (0.0 -> 1.0)
        targetFace.eyeHeight = 25.0f - (20.0f * progress); 
        
        // Hiệu ứng chép miệng và gật gù chậm lại (Chỉ mấp máy 1-2 lần rất từ tốn)
        targetFace.offsetY = 8 + 2 * sin(elapsed * 0.008);    
        targetFace.mouthHeight = 5 + 4 * sin(elapsed * 0.008); 
        targetFace.mouthWidth = 22 + 5 * cos(elapsed * 0.008);
      } else if (elapsed > 1200) {
        // Giai đoạn Ngáp (Yawn): Miệng mở to chữ O, mắt nhắm hờ, đầu hơi ngước
        targetFace = stateNormal; 
        targetFace.eyeAngle = 0; 
        targetFace.eyeHeight = 25;   // Mắt sụp xuống một nửa
        targetFace.offsetY = -5;     // Đầu ngước lên ngáp
        targetFace.mouthHeight = 35; // Mở mồm to hết cỡ
        targetFace.mouthWidth = 20;  // Mồm thu hẹp lại chữ O
      } else {
        // Giai đoạn chớp mắt lờ đờ ban đầu
        targetFace = stateNormal; 
        targetFace.eyeAngle = 0; // Quan trọng: Ép mắt phẳng ngang lờ đờ để mí sụp không bị tạo hình chữ V (giận dữ)
        targetFace.offsetY = 8;  // Đầu đã bắt đầu cúi gục nhẹ
      }
      break;
    }
    case 6: targetFace = stateAngry; break;
    case 7: targetFace = stateSurprised; break;
    case 8: targetFace = stateDoubt; break;
    case 9: targetFace = stateCry; break;
    case 10: targetFace = stateDizzy; break;
    case 11: targetFace = stateWink; break;
    case 12: targetFace = stateNormal; break; // Hoạt ảnh liếc xung quanh sẽ được thêm riêng trong effX, effY
    case 13: targetFace = statePanic; break;
    case 14: targetFace = stateSmug; break;
    case 15: targetFace = stateScan; break;
    case 16: targetFace = stateBored; break;
    case 17: targetFace = stateLove; break;
    case 18: targetFace = stateGlitch; break;
    case 19: targetFace = stateSus; break;
    case 20: targetFace = stateFurious; break;
    case 21: targetFace = statePetting; break;
  }

  // 1. Tính toán hiệu ứng Lerp mượt mà (Đã chuyển vào Animation.cpp)
  updateFaceLogic();

  // 2. Vẽ ra Sprite ẩn (Đã chuyển vào Animation.cpp)
  renderToScreen();

  // 3. Đẩy Sprite ra màn hình LCD thật (Hardware Write)
  canvasSprite.pushSprite(0, 0);

  // 4. Tắt đèn GPIO 1 sau 50ms
  if (isStateBlinking && (millis() - stateBlinkStartTime > 50)) {
    digitalWrite(1, LOW);
    isStateBlinking = false;
  }

  // Nhường CPU để Watchdog không bị kích hoạt
  vTaskDelay(pdMS_TO_TICKS(10)); 
}
