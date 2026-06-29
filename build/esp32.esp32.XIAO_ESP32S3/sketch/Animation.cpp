#line 1 "C:\\rust\\face_rbot\\Animation.cpp"
#include "Animation.h"
void updateFaceLogic() {
  // Điều chỉnh tốc độ chuyển trạng thái (Animation Timing Tùy chỉnh)
  float currentLerp = 0.2; // Tăng default từ 0.1 lên 0.2 để mặt phản ứng lanh lẹ hơn
  if (targetFace.eyeHeight == stateSleep.eyeHeight) {
    currentLerp = 0.35; // Ngủ: Chuyển trạng thái nhanh hơn nữa
  }
  else if (targetFace.eyeAngle == -8) {
    currentLerp = 0.5;  // Chép miệng sau ngáp: Tốc độ nội suy cực cao để bắt kịp sóng Sin
  }
  else if (targetFace.eyeHeight == stateSurprised.eyeHeight) {
    currentLerp = 0.6;  // Ngạc nhiên: Giật bắn mình mở to mắt (Cực nhanh)
  }
  else if (targetFace.eyeAngle == stateAngry.eyeAngle) {
    currentLerp = 0.5;  // Giận dữ: Quắc mắt dứt khoát
  }
  else if (targetFace.eyeHeight == stateDoubt.eyeHeight) {
    currentLerp = 0.35; // Nghi ngờ: Nhanh hơn
  }

  currentFace.eyeShapeType = targetFace.eyeShapeType; 
  currentFace.eyeWidth    += (targetFace.eyeWidth    - currentFace.eyeWidth)    * currentLerp;
  currentFace.eyeHeight   += (targetFace.eyeHeight   - currentFace.eyeHeight)   * currentLerp;
  currentFace.eyeRadius   += (targetFace.eyeRadius   - currentFace.eyeRadius)   * currentLerp;
  currentFace.eyeAngle    += (targetFace.eyeAngle    - currentFace.eyeAngle)    * currentLerp;
  currentFace.glowSize    += (targetFace.glowSize    - currentFace.glowSize)    * currentLerp;
  currentFace.innerShadow += (targetFace.innerShadow - currentFace.innerShadow) * currentLerp;
  currentFace.mouthHeight += (targetFace.mouthHeight - currentFace.mouthHeight) * currentLerp;
  currentFace.mouthWidth  += (targetFace.mouthWidth  - currentFace.mouthWidth)  * currentLerp;
  currentFace.mouthGlowSize += (targetFace.mouthGlowSize - currentFace.mouthGlowSize) * currentLerp;
  currentFace.mouthInnerShadow += (targetFace.mouthInnerShadow - currentFace.mouthInnerShadow) * currentLerp;
  currentFace.offsetX     += (targetFace.offsetX     - currentFace.offsetX)     * currentLerp;
  currentFace.offsetY     += (targetFace.offsetY     - currentFace.offsetY)     * currentLerp;

  // Xử lý Animation Overrides Weights (Lerp mượt mà cho các hiệu ứng phức tạp)
  float targetSusWeight = (targetEmotionCode == 19) ? 1.0f : 0.0f;
  susWeight += (targetSusWeight - susWeight) * currentLerp;
  
  float targetSmugWeight = (targetEmotionCode == 14) ? 1.0f : 0.0f;
  smugWeight += (targetSmugWeight - smugWeight) * currentLerp;

  // Xử lý Blink Override độc lập (Không làm hỏng State gốc)
  unsigned long now = millis();
  
  // Tùy chỉnh tốc độ nhắm mắt: Ngạc nhiên thì chớp cực nhanh (50ms) để không bỏ lỡ khoảnh khắc
  unsigned long blinkDuration = 150; 
  if (targetFace.eyeHeight == stateSurprised.eyeHeight) {
    blinkDuration = 50;
  } else if (targetEmotionCode == 5 && targetFace.eyeHeight != stateSleep.eyeHeight) {
    blinkDuration = 600; // Buồn ngủ: Mí mắt nặng trĩu, sụp mí rất lâu (600ms) mới mở lên lại
  } else if (targetEmotionCode == 16) {
    blinkDuration = 800; // Bored: Mở mắt chậm lờ đờ
  } else if (targetEmotionCode == 18) {
    blinkDuration = random(20, 100); // Glitch: Giật nhanh
  }

  // Khóa chớp mắt tự động khi đang thực hiện Wink (11) hoặc Dizzy (10) để không bị đứt đoạn animation
  if (targetEmotionCode == 11 || targetEmotionCode == 10) {
    lastBlinkTime = now; 
    targetBlinkFactor = 1.0;
  } else if (now - lastBlinkTime > nextBlinkDelay) {
    targetBlinkFactor = 0.05; // Ép chiều cao về 5%
    if (now - lastBlinkTime > nextBlinkDelay + blinkDuration) { // Giữ mắt nhắm trong blinkDuration
      targetBlinkFactor = 1.0; // Mở mắt
      lastBlinkTime = now;
      
      // Xử lý chớp mắt liên tục khi buồn ngủ hoặc ngạc nhiên
      if (sleepBlinkCount > 0) {
        sleepBlinkCount--;
        nextBlinkDelay = 300; // Nháy lại ngay lập tức sau 300ms
      } else if (targetEmotionCode == 18) {
        nextBlinkDelay = random(100, 500); // Giật chớp liên tục
      } else if (targetFace.eyeHeight == stateSurprised.eyeHeight) {
        nextBlinkDelay = random(1000, 2500); // Ngạc nhiên: tần suất chớp mắt cao hơn (1-2.5s)
      } else {
        nextBlinkDelay = random(2000, 6000); // Ngẫu nhiên 2s đến 6s
      }
    }
  }
  
  // Tốc độ khép mí: Ngạc nhiên chớp lẹ hơn bình thường (0.7 so với 0.5)
  float blinkSpeed = 0.5f;
  if (targetFace.eyeHeight == stateSurprised.eyeHeight) {
    blinkSpeed = 0.7f;
  } else if (targetEmotionCode == 5 && targetFace.eyeHeight != stateSleep.eyeHeight) {
    blinkSpeed = 0.15f; // Buồn ngủ: Mí mắt sụp xuống chậm rãi, lờ đờ
  } else if (targetEmotionCode == 16) {
    blinkSpeed = 0.1f; // Bored: Tốc độ nháy mắt cực kỳ lề mề
  }
  
  blinkFactor += (targetBlinkFactor - blinkFactor) * blinkSpeed; 
}
void drawEye(float centerX, float centerY, bool isRightEye, float scale3D, float extraAngle, float customBlink, float pitchFactor) {
  eyeSprite.fillSprite(TFT_BLACK);
  float pivotX = 60, pivotY = 60;
  eyeSprite.setPivot(pivotX, pivotY);

  // ĐỊNH NGHĨA DẢI MÀU (Bảng màu tĩnh theo chuẩn RGB888 Hex)
  uint32_t colorTop    = 0x00DC00; // (0, 220, 0)
  uint32_t colorMid    = 0x00D700; // (0, 215, 0)
  uint32_t colorBot    = 0x00D200; // (0, 210, 0)
  uint32_t shadowColor = 0x00C800; // (0, 200, 0)

  if (targetEmotionCode == 20) { // Furious -> Lửa giận (Dùng chuẩn RGB)
    colorTop    = 0xFF0000; // Đỏ
    colorMid    = 0xDD0000; // Đỏ sậm
    colorBot    = 0xBB0000; // Đỏ tối
    shadowColor = 0x880000; // Đỏ đậm
  } else if (targetEmotionCode == 18 && random(10) > 8) { // Glitch -> Nhiễu màu ngẫu nhiên
    colorTop = random(0xFFFFFF);
    colorBot = random(0xFFFFFF);
  }

  float actualBlink = (customBlink >= 0.0f) ? customBlink : blinkFactor;
  float w = currentFace.eyeWidth * scale3D;
  float h = currentFace.eyeHeight * actualBlink * scale3D; // Áp dụng Blink Override & 3D Scale
  if (h < 4) h = 4; // Guardrail: tối thiểu 4 pixel để thuật toán vát góc không sập
  float shape = currentFace.eyeShapeType;

  // 1. Vẽ Bóng Giả (Dark Blue) làm nền
  drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY, w - 4, h, shape, shadowColor, shadowColor, false, pitchFactor);

  if (targetEmotionCode == 10) {
    // 2. Vẽ Hiệu ứng Xoáy Thôi Miên
    uint32_t color = 0xFFFF00; // Dùng màu Vàng rực (Yellow) để tăng tương phản tuyệt đối trên nền mắt!

    float spin = millis() * 0.3f; // Tốc độ xoay
    for (int r = 5; r < (w / 2) - 2; r += 6) {
      eyeSprite.drawArc(pivotX, pivotY, r, r - 2, spin - r * 15, spin - r * 15 + 180, color);
    }
  } else if (targetEmotionCode == 15) {
    // 2. GIẢI PHÁP CHUYỂN MÀU PHÂN LỚP
    drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 1, w,     h,     shape, colorBot, colorBot, false, pitchFactor);
    drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 2, w - 2, h - 2, shape, colorMid, colorMid, false, pitchFactor);
    drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 3, w - 4, h - 4, shape, colorTop, colorTop, false, pitchFactor);

    // Vẽ thanh quét (Scanner bar) xoay vòng quanh tâm mắt như Radar thật
    float scanAngle = (millis() % 2000) / 2000.0f * 2 * PI; // Quay 1 vòng mỗi 2 giây
    float lineEndX = pivotX + cos(scanAngle) * (w / 2 - 2);
    float lineEndY = pivotY + sin(scanAngle) * (h / 2 - 2);
    
    // Quét một tia sáng màu Cyan từ tâm ra rìa
    uint32_t radarColor = 0x00FFFF; // Cyan
    eyeSprite.drawLine(pivotX, pivotY, lineEndX, lineEndY, TFT_WHITE);
    
    // Tạo hiệu ứng đuôi mờ mờ (trail) phía sau tia quét chính
    for (int i = 1; i <= 3; i++) {
      float tailAngle = scanAngle - (i * 0.1f);
      float tailEndX = pivotX + cos(tailAngle) * (w / 2 - 2);
      float tailEndY = pivotY + sin(tailAngle) * (h / 2 - 2);
      eyeSprite.drawLine(pivotX, pivotY, tailEndX, tailEndY, radarColor);
    }
  } else {
    // 2. GIẢI PHÁP CHUYỂN MÀU PHÂN LỚP (CONCENTRIC LAYERS)
    // Thu hẹp khoảng cách các lớp (w-2, w-4) để viền tối mỏng lại, lõi sáng to ra
    drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 1, w,     h,     shape, colorBot, colorBot, false, pitchFactor);
    drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 2, w - 2, h - 2, shape, colorMid, colorMid, false, pitchFactor);
    drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 3, w - 4, h - 4, shape, colorTop, colorTop, false, pitchFactor);
  }

  // Xoay và in ra màn hình
  canvasSprite.setPivot(centerX, centerY);
  float angle = isRightEye ? -(currentFace.eyeAngle + extraAngle) : (currentFace.eyeAngle + extraAngle);
  eyeSprite.pushRotated(&canvasSprite, angle, TFT_BLACK); 
}
void renderToScreen() {
  canvasSprite.fillSprite(tft.color565(0, 0, 0));

  // Áp dụng offsetX, offsetY để mô phỏng trục xoay cổ
  float effX = currentFace.offsetX;
  float effY = currentFace.offsetY;
  
  float leftBlink = -1.0f;
  float rightBlink = -1.0f;
  float leftEyeScale = 1.0f; 
  float rightEyeScale = 1.0f;
  float cryHiccupPower = 0.0f; // Sức mạnh của nhịp nấc để đồng bộ mắt và miệng

  // --- BỔ SUNG ANIMATION ĐỘNG CHO TẤT CẢ CÁC TRẠNG THÁI TĨNH CÒN LẠI ---
  // 0 (Idle) & 1 (Normal): Nhịp thở nhẹ nhàng, đều đặn
  if (targetEmotionCode == 0 || targetEmotionCode == 1) {
    effY += sin(millis() / 400.0f) * 3.0f; // Nhịp thở nhanh hơn (400) và biên độ rõ (3.0)
  }

  // 4 (Talk): Gật gù, nhún nhảy khi nói chuyện
  if (targetEmotionCode == 4) {
    effY += sin(millis() / 150.0f) * 3.5f;
  }

  // 5 (Sleep): Thở dốc nhịp dài, chậm và sâu (bụng phập phồng)
  if (targetEmotionCode == 5) {
    effY += sin(millis() / 1500.0f) * 4.0f;
    effX += cos(millis() / 1500.0f) * 2.0f; // Đầu hơi nghiêng
  }

  // 8 (Doubt): Lắc nhẹ đầu sang hai bên, kiểu "không chắc lắm"
  if (targetEmotionCode == 8) {
    effX += sin(millis() / 400.0f) * 4.0f;
  }

  // 11 (Wink): Nhún 1 cái thật nhẹ khi nháy mắt
  if (targetEmotionCode == 11) {
    effY += sin(millis() / 200.0f) * 2.0f;
  }

  // 15 (Scan): Quét ngang nhè nhẹ (như camera an ninh xoay)
  if (targetEmotionCode == 15) {
    effX += sin(millis() / 1000.0f) * 12.0f;
  }

  // 16 (Bored): Gục gặc chán nản
  if (targetEmotionCode == 16) {
    effY += sin(millis() / 600.0f) * 2.5f;
  }

  // 2 (Happy): Cười hớn hở nảy lên nảy xuống
  if (targetEmotionCode == 2) {
    effY += sin(millis() / 150.0f) * 6.0f;
  }
  
  // 3 (Sad): Thở dài chậm chạp, cúi gằm mặt
  if (targetEmotionCode == 3) {
    effY += sin(millis() / 600.0f) * 3.0f;
  }

  // 6 (Angry): Thở dốc, phập phồng và rung nhẹ vì tức giận
  if (targetEmotionCode == 6) {
    effX += random(-1, 2); 
    effY += sin(millis() / 100.0f) * 2.5f;
  }

  // 7 (Surprised): Bàng hoàng, nhịp tim đập nhanh nhấp nhô
  if (targetEmotionCode == 7) {
    effY += sin(millis() / 100.0f) * 3.0f; 
  }

  // 9 (Cry): Khóc run người, thỉnh thoảng nấc cụt (đôi khi nấc đúp 2 cái)
  if (targetEmotionCode == 9) {
    effY += sin(millis() / 300.0f) * 2.0f; // Run nhẹ
    
    unsigned long cycle = millis() % 2500;
    unsigned long cycleIndex = millis() / 2500;
    
    // Nhịp nấc thứ nhất (luôn xảy ra)
    if (cycle < 250) { 
      float hiccupPhase = cycle / 250.0f; 
      cryHiccupPower = sin(hiccupPhase * PI);
      effY -= cryHiccupPower * 8.0f;
    }
    // Nhịp nấc thứ hai (xảy ra thi thoảng - cứ 3 chu kỳ thì bị 1 lần nấc đúp)
    else if (cycleIndex % 3 == 0 && cycle > 350 && cycle < 600) {
      float hiccupPhase = (cycle - 350) / 250.0f; 
      cryHiccupPower = sin(hiccupPhase * PI);
      effY -= cryHiccupPower * 8.0f;
    }
  }

  // 14 (Smug): Đắc ý lắc lư cái đầu qua lại (Hình oval dẹt)
  // Lưu ý: Smug (14) đã có lerp mouthAngle, phần này chỉ thêm lắc lư đầu
  if (targetEmotionCode == 14) {
    effX += sin(millis() / 250.0f) * 5.0f; 
    effY += cos(millis() / 250.0f) * 2.0f;
  }

  // Hiệu ứng Chóng mặt (Dizzy - Code 10): Xoay vòng vòng
  if (targetEmotionCode == 10) { 
    effX += sin(millis() / 150.0f) * 8.0f; // Giảm biên độ quay
    effY += cos(millis() / 150.0f) * 8.0f;
  }
  
  // Hiệu ứng Hoảng sợ (Panic - Code 13): Rung lắc liên tục
  if (targetEmotionCode == 13) {
    effX += random(-2, 4); 
    effY += random(-2, 4);
  }

  // Love (17): Bồng bềnh, nhịp đập
  if (targetEmotionCode == 17) {
    float pulse = sin(millis() / 150.0f) * 0.15f; 
    leftEyeScale += pulse;
    rightEyeScale += pulse;
    effY += sin(millis() / 400.0f) * 5.0f; // Bồng bềnh
  }

  // Petting (21): Vuốt ve, đầu xoay nhẹ rung rinh mãn nguyện
  if (targetEmotionCode == 21) {
    effX += sin(millis() / 200.0f) * 3.0f; // Lắc lư nhẹ
    effY += cos(millis() / 250.0f) * 2.0f; // Nhấp nhô hưởng thụ
    
    // Thêm chút rung động giống như con mèo đang kêu gừ gừ
    effX += random(-1, 2) * 0.5f;
    effY += random(-1, 2) * 0.5f;
  }

  // Glitch (18): Lag giật cục
  if (targetEmotionCode == 18) {
    if (random(10) > 7) {
      effX += random(-15, 16);
      effY += random(-15, 16);
    }
  }

  float leftAngle = 0.0f;
  float rightAngle = 0.0f;

  // Sus (19): Ánh mắt phán xét (Nghiêng đầu nhẹ, liếc xéo, nheo cả 2 mắt)
  // Dùng susWeight để Lerp mượt mà mọi thông số, tránh hiện tượng nhảy (Snap)
  if (susWeight > 0.01f) {
    leftBlink = blinkFactor * (1.0f - susWeight) + 0.55f * susWeight;
    rightBlink = blinkFactor * (1.0f - susWeight) + 0.55f * susWeight;
    leftAngle = 5.0f * susWeight;
    rightAngle = -5.0f * susWeight;
    effX += (15.0f + sin(millis() / 600.0f) * 4.0f) * susWeight; // Thêm lắc nhẹ dò xét
    effY -= 5.0f * susWeight;
  }

  // Furious (20): Rung nhẹ (sôi máu)
  if (targetEmotionCode == 20) {
    effX += random(-1, 2);
    effY += random(-1, 2);
  }
  
  // Nhìn xung quanh (LookAround - Code 12) kiểu nhìn bất định (Random Wandering)
  static float lookAroundOffsetX = 0.0f;
  static float lookAroundOffsetY = 0.0f;
  static float targetLookX = 0.0f;
  static float targetLookY = 0.0f;
  static unsigned long lookWaitTime = 0;
  static bool isLookingWaiting = false;
  static unsigned long currentLookDelay = 1500;

  if (targetEmotionCode == 12) {
    float diffX = targetLookX - lookAroundOffsetX;
    float diffY = targetLookY - lookAroundOffsetY;

    // Kiểm tra xem mắt đã trôi đến gần mục tiêu (khoảng cách < 1.0) chưa
    if (abs(diffX) < 1.0f && abs(diffY) < 1.0f) {
      if (!isLookingWaiting) {
        // Vừa mới đến đích, khóa lại và bắt đầu đếm giờ chờ
        isLookingWaiting = true;
        lookWaitTime = millis();
      } else {
        // Đang ở đích, kiểm tra xem đã chờ đủ 1-2s chưa
        if (millis() - lookWaitTime > currentLookDelay) {
          targetLookX = random(-35, 36); // Liếc qua lại trục X
          targetLookY = random(-20, 21); // Liếc lên xuống trục Y
          currentLookDelay = random(1000, 2500); // Random chờ 1s - 2.5s cho lần tới
          isLookingWaiting = false; // Bắt đầu di chuyển
        }
      }
    } else {
      isLookingWaiting = false;
    }

    // Tính toán khoảng cách nội suy để trôi mượt mà (tăng hệ số để mắt liếc nhanh dứt khoát)
    float stepX = diffX * 0.15f; 
    float stepY = diffY * 0.15f; 
    
    // Giới hạn tốc độ liếc tối đa được nâng lên để ánh mắt lanh lẹ hơn
    if (stepX > 3.5f) stepX = 3.5f; if (stepX < -3.5f) stepX = -3.5f;
    if (stepY > 3.5f) stepY = 3.5f; if (stepY < -3.5f) stepY = -3.5f;
    
    lookAroundOffsetX += stepX;
    lookAroundOffsetY += stepY;
  } else {
    // Khi thoát trạng thái, trôi về giữa nhanh chóng
    float stepX = (0.0f - lookAroundOffsetX) * 0.15f;
    float stepY = (0.0f - lookAroundOffsetY) * 0.15f;
    if (stepX > 3.5f) stepX = 3.5f; if (stepX < -3.5f) stepX = -3.5f;
    if (stepY > 3.5f) stepY = 3.5f; if (stepY < -3.5f) stepY = -3.5f;
    lookAroundOffsetX += stepX; 
    lookAroundOffsetY += stepY;
  }
  effX += lookAroundOffsetX;
  effY += lookAroundOffsetY;

  // Hiệu ứng Nháy mắt (Wink - Code 11): Liếc mắt trái -> nháy mắt phải -> trả về
  static float winkOffset = 0.0f;

  if (targetEmotionCode == 11) {
    long elapsed = millis() - winkStartTime;
    float targetLook = winkDirection ? 40.0f : -40.0f; // winkDirection=true -> nhìn phải, false -> nhìn trái

    // Giai đoạn 1: Liếc sang một bên (Luôn nội suy mượt, không snap cứng để tránh bị "khựng")
    if (elapsed <= 1000) {
      winkOffset += (targetLook - winkOffset) * 0.2f; 
    } else {
      winkOffset += (0.0f - winkOffset) * 0.2f; // Trôi về
    }
    
    // Giai đoạn 2: Nháy mắt NGƯỢC LẠI hướng liếc (bắt đầu khi mắt đã liếc một chút)
    if (elapsed > 300 && elapsed <= 450) {
      // Đóng mắt cực nhanh (Ease-in)
      float t = (elapsed - 300) / 150.0f;
      float blinkVal = 1.0f - (t * t); 
      if (winkDirection) leftBlink = blinkVal; else rightBlink = blinkVal;
    } else if (elapsed > 450 && elapsed <= 650) {
      // Mở mắt từ từ (Ease-out)
      float t = (elapsed - 450) / 200.0f;
      float blinkVal = t * t; 
      if (winkDirection) leftBlink = blinkVal; else rightBlink = blinkVal;
    } else if (elapsed > 650 && elapsed <= 1000) {
      // Giữ trạng thái mở bình thường
      if (winkDirection) leftBlink = 1.0f; else rightBlink = 1.0f;
    }
    
    if (leftBlink >= 0.0f && leftBlink < 0.05f) leftBlink = 0.05f; 
    if (rightBlink >= 0.0f && rightBlink < 0.05f) rightBlink = 0.05f; 

    // Giai đoạn 3: Tự động trả về trạng thái Normal (1) khi hoạt cảnh hoàn tất
    if (elapsed > 1200) {
      targetEmotionCode = 1;
    }
  } else {
    winkOffset += (0.0f - winkOffset) * 0.15f;
  }
  effX += winkOffset;

  // Tọa độ mắt sau khi cộng các offset độc lập
  float eyeLx = 60 + effX;
  float eyeRx = 180 + effX;
  float eyeY  = 90 + effY;

  // Giả lập chiều sâu 3D (Parallax): Mắt ở hướng nhìn ngược lại sẽ to hơn
  leftEyeScale += (effX * 0.002f); 
  rightEyeScale -= (effX * 0.002f);
  
  // Phối cảnh 3D hình thang (Pitch): Ngước lên (effY < 0) thì trên bé lại. Cúi xuống (effY > 0) thì dưới bé lại.
  float eyePitchFactor = -effY * 0.015f; 

  // Khi nấc (Cry), nheo tịt mắt lại do cơ mặt co giật
  if (cryHiccupPower > 0.01f) {
    float blinkTarget = blinkFactor - (cryHiccupPower * 0.8f);
    if (blinkTarget < 0.05f) blinkTarget = 0.05f;
    leftBlink = blinkTarget;
    rightBlink = blinkTarget;
  }

  drawEye(eyeLx, eyeY, false, leftEyeScale, leftAngle, leftBlink, eyePitchFactor); 
  drawEye(eyeRx, eyeY, true, rightEyeScale, rightAngle, rightBlink, eyePitchFactor);

  // Hiệu ứng Khóc (Cry - Code 9): Rơi nước mắt
  if (targetEmotionCode == 9) { 
    float tearPhase = (millis() % 1500) / 1500.0f; // Vòng lặp 1.5s
    float tearY = eyeY + 25 + tearPhase * 20.0f; 
    float tearW = 8.0f * (1.0f - tearPhase); // Nhỏ dần
    float tearH = 12.0f * (1.0f - tearPhase);
    
    uint32_t tearColor = 0x00FFFF; // Màu Cyan
    
    if (tearW > 2) {
      drawGradientAsymmetricRect(&canvasSprite, eyeLx + 10, tearY, tearW, tearH, 2, tearColor, tearColor, false);
    }
    
    // Mắt phải rơi lệch nhịp (độc lập với mắt trái)
    float tearPhaseR = ((millis() + 500) % 1500) / 1500.0f;
    float tearYR = eyeY + 25 + tearPhaseR * 20.0f;
    float tearWR = 8.0f * (1.0f - tearPhaseR);
    float tearHR = 12.0f * (1.0f - tearPhaseR);
    if (tearWR > 2) {
      drawGradientAsymmetricRect(&canvasSprite, eyeRx - 10, tearYR, tearWR, tearHR, 2, tearColor, tearColor, false);
    }
  }

  if (currentFace.mouthHeight > 0.5) {
    // Miệng đi theo mắt (theo effX, effY) thay vì chỉ offsetX cứng
    float mouthX = 120 + effX;
    float mouthY = 125 + effY;

    // Phối cảnh 3D cho miệng: ngước lên (effY < 0) -> cằm gần lại -> miệng to ra. Cúi xuống (effY > 0) -> cằm lùi xa -> miệng bé lại.
    float mouthDepthScale = 1.0f - (effY * 0.005f);
    
    float w = currentFace.mouthWidth * mouthDepthScale;
    float h = currentFace.mouthHeight * mouthDepthScale;
    
    // --- OVERRIDE: TALK ANIMATION (Mấp máy môi) ---
    // Chỉ kích hoạt khi trạng thái hiện tại là Talk (4)
    if (targetEmotionCode == 4) {
      // Giảm tốc độ nói (chia cho 150.0f thay vì 80.0f) để khớp với nhịp điệu tự nhiên hơn
      float talkPhase = millis() / 150.0f;
      float talkFactor = 0.3f + 0.7f * abs(sin(talkPhase) * sin(talkPhase * 0.6f));
      h = h * talkFactor;
    }
    
    // Khi khóc nấc, miệng há ra một chút để "ngáp hơi" (Gasping for air)
    if (cryHiccupPower > 0.01f) {
      h += cryHiccupPower * 6.0f;  // Giảm biên độ há miệng xuống (trước đây là 15.0f)
      // Miệng sẽ hơi thu nhỏ chiều rộng lại khi há to ra (chữ O)
      w -= cryHiccupPower * 2.0f;
    }
    
    if (h < 2) h = 2; // Guardrail: giữ miệng không bị sập hoàn toàn

    // Tính toán độ bo tròn tức thời (0% -> 50%) dựa trên chiều cao thực tế lúc đang nói
    float topRadiusFactor = 0.0f; 
    float unscaledH = h / mouthDepthScale; // Dùng h gốc để tính bo tròn tránh bị bóp méo
    if (unscaledH > 8.0f) {
      topRadiusFactor = (unscaledH - 8.0f) * 0.0185f; // Bắt đầu bo tròn khi h > 8
      if (topRadiusFactor > 0.5f) topRadiusFactor = 0.5f; // Đạt đỉnh tròn trịa (50%)
    }

    // Đồng bộ bảng màu với Mắt (RGB888 Hex)
    uint32_t colorTop    = 0x00DC00;
    uint32_t colorMid    = 0x00D700;
    uint32_t colorBot    = 0x00D200;
    uint32_t shadowColor = 0x00C800;

    if (targetEmotionCode == 20) { // Furious -> Lửa giận (Dùng chuẩn RGB)
      colorTop    = 0xFF0000;
      colorMid    = 0xDD0000;
      colorBot    = 0xBB0000;
      shadowColor = 0x880000;
    } else if (targetEmotionCode == 18 && random(10) > 8) {
      colorTop = random(0xFFFFFF);
      colorBot = random(0xFFFFFF);
    }

    // Cả miệng cũng chịu hiệu ứng Trapezoid Pitch như mắt để đồng bộ (cùng nghiêng mặt)
    float mouthPitchFactor = -effY * 0.015f;

    // Hiệu ứng Smug (14) & Sus (19): Nhếch mép nghiêng mồm
    float mouthAngle = 0.0f;
    float maxWeight = (smugWeight > susWeight) ? smugWeight : susWeight;
    if (maxWeight > 0.01f) {
      mouthAngle = 10.0f * maxWeight;
    }

    if (mouthAngle != 0.0f) {
      eyeSprite.fillSprite(TFT_BLACK);
      float pivotX = 60, pivotY = 60;
      eyeSprite.setPivot(pivotX, pivotY);
      
      drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY, w - 4, h, topRadiusFactor, shadowColor, shadowColor, true, mouthPitchFactor);
      drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 1, w,     h,     topRadiusFactor, colorBot, colorBot, true, mouthPitchFactor);
      if (h > 2) drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 2, w - 2, h - 2, topRadiusFactor, colorMid, colorMid, true, mouthPitchFactor);
      if (h > 4) drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY - 3, w - 4, h - 4, topRadiusFactor, colorTop, colorTop, true, mouthPitchFactor);
      
      canvasSprite.setPivot(mouthX, mouthY);
      eyeSprite.pushRotated(&canvasSprite, mouthAngle, TFT_BLACK);
    } else {
      // Truyền topRadiusFactor vào tham số thứ 6 (shapeType) để đảm bảo mọi lớp đồng bộ độ bo góc
      drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY, w - 4, h, topRadiusFactor, shadowColor, shadowColor, true, mouthPitchFactor);
      
      // Chuyển màu phân lớp cho Miệng (viền mỏng hơn)
      drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY - 1, w,     h,     topRadiusFactor, colorBot, colorBot, true, mouthPitchFactor);
      if (h > 2) drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY - 2, w - 2, h - 2, topRadiusFactor, colorMid, colorMid, true, mouthPitchFactor);
      if (h > 4) drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY - 3, w - 4, h - 4, topRadiusFactor, colorTop, colorTop, true, mouthPitchFactor);
    }
  }

  canvasSprite.pushSprite(0, 0);
}

unsigned long lastUpdate = 0;
unsigned long stateChangeTimer = 0;
unsigned long nextStateDelay = 2000;

