#include <Arduino.h>
#line 1 "C:\\rust\\face_rbot\\face_rbot.ino"
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX(void) {
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
};

LGFX tft;
LGFX_Sprite canvasSprite(&tft);
LGFX_Sprite eyeSprite(&tft);

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
  float offsetX; // Tọa độ X giả lập chuyển động cổ/mắt
  float offsetY; // Tọa độ Y giả lập chuyển động cổ/mắt
};

// Khớp 100% với file HTML của user, bổ sung offsetX, offsetY
const FaceState stateNormal    = {0, 40, 50, 20,  7, 4, 14,  7, 40, 2, 4,   0,   0};
const FaceState stateIdle      = {0, 40, 50, 20,  7, 4, 14,  0,  0, 0, 0,   0,   0};  
const FaceState stateHappy     = {1, 45, 25, 20, 12, 4, 14, 25, 55, 2, 4,   0,  -5}; 
const FaceState stateSad       = {0, 35, 40, 15,-10, 4, 14,  4, 20, 2, 4,   0,  15}; 
const FaceState stateLookLeft  = {0, 35, 50, 20,  5, 4, 14,  5, 25, 2, 4, -30,   0};
const FaceState stateLookRight = {0, 35, 50, 20,  5, 4, 14,  5, 25, 2, 4,  30,   0};
const FaceState stateTalk      = {0, 38, 55, 18,  7, 4, 14, 35, 30, 2, 4,   0,  -2};
const FaceState stateSleep     = {0, 40,  7,  3,  0, 4, 14,  0,  0, 0, 0,   0,  15}; // Mắt khép nhỏ, đầu cúi xuống (offsetY=15)
const FaceState stateAngry     = {0, 40, 25, 10, 25, 4, 14,  5, 40, 2, 4,   0,   5}; // Mắt dẹt, góc nghiêng gắt, miệng rộng và bẹt
const FaceState stateSurprised = {0, 50, 60, 25,  0, 4, 14, 40, 25, 2, 4,   0, -15}; // Mắt mở to tròn, miệng chữ O dài, đầu giật lên
const FaceState stateDoubt     = {0, 35, 15,  5,  0, 4, 14,  4, 15, 2, 4,  25,   5}; // Mắt híp (squint), liếc sang một bên nghi ngờ
const FaceState stateCry       = {0, 35, 20, 10,-15, 4, 14, 7, 20, 2, 4,   0,  10}; // Mắt nheo xuống, mếu máo
const FaceState stateDizzy     = {0, 40, 50, 20,  0, 4, 14, 20, 20, 2, 4,   0,   0}; // Hình dáng bình thường nhưng sẽ quay vòng vòng
const FaceState stateWink      = {0, 40, 50, 20,  7, 4, 14, 15, 40, 2, 4,   0,   0}; // Một mắt nhắm một mắt mở (xử lý logic riêng)
const FaceState statePanic     = {0, 65, 65, 32,  0, 4, 14,  5, 15, 2, 4,   0,   0}; // Mắt mở to tròn hết cỡ, miệng chữ O nhỏ
const FaceState stateSmug      = {2, 40, 30, 15,-15, 4, 14,  5, 25, 2, 4,  10,-10}; // Bán nguyệt dưới, liếc ngước, miệng nhếch lệch
const FaceState stateScan      = {3, 40,  5,  2,  0, 4, 14,  2, 40, 2, 4,   0,   0}; // Mắt viên thuốc dẹt ngang, mồm dẹt mỏng
const FaceState stateBored     = {3, 40, 15,  7,  0, 4, 14,  5, 20, 2, 4,   0,  25}; // Mắt nửa vời, cúi gập gục đầu

FaceState currentFace = stateNormal;
FaceState targetFace = stateIdle;

// Giao tiếp liên lõi (Inter-core Communication)
volatile int targetEmotionCode = 1; // Mặc định là Normal (1)
int lastEmotionCode = 1; // Dùng trên Core 1 để phát hiện chuyển đổi trạng thái
unsigned long winkStartTime = 0;
unsigned long sleepStartTime = 0;
bool winkDirection = false; // Luân phiên hướng nháy mắt (false=trái, true=phải)

// ==========================================
// HỆ THỐNG AI: Q-LEARNING & MOCK SENSORS
// ==========================================

// 1. Không gian trạng thái rời rạc (Quantized State Space)
enum TempState { COLD = 0, NORMAL = 1, HOT = 2 };
enum SoundState { QUIET = 0, NOISY = 1 };
enum TouchState { UNTOUCHED = 0, TOUCHED = 1 };

const int NUM_STATES = 3 * 2 * 2; // 12 Trạng thái
const int NUM_ACTIONS = 17; // 17 Biểu cảm (Idle, Normal, Happy, Sad, Talk, Sleep, Angry, Surprised, Doubt, Cry, Dizzy, Wink, LookAround, Panic, Smug, Scan, Bored)

// 2. Q-Table (Bộ nhớ Kinh nghiệm)
float qTable[NUM_STATES][NUM_ACTIONS] = {0.0}; 

// 3. Siêu tham số Học tăng cường
const float LEARNING_RATE = 0.1f;
const float DISCOUNT_FACTOR = 0.9f;
float explorationRate = 0.2f; // 20% khám phá ngẫu nhiên

// 4. Biến môi trường giả lập (Mock Sensors)
int currentTemp = NORMAL;
int currentSound = QUIET;
int currentTouch = UNTOUCHED;

// Hàm chuyển đổi tổ hợp cảm biến thành 1 mã trạng thái (0-11)
#line 118 "C:\\rust\\face_rbot\\face_rbot.ino"
int getStateIndex(int temp, int sound, int touch);
#line 123 "C:\\rust\\face_rbot\\face_rbot.ino"
void readMockSensors();
#line 131 "C:\\rust\\face_rbot\\face_rbot.ino"
float calculateReward(int state, int action);
#line 161 "C:\\rust\\face_rbot\\face_rbot.ino"
void learn(int state, int action, float reward, int nextState);
#line 173 "C:\\rust\\face_rbot\\face_rbot.ino"
void AITask(void *pvParameters);
#line 214 "C:\\rust\\face_rbot\\face_rbot.ino"
void updateFaceLogic();
#line 293 "C:\\rust\\face_rbot\\face_rbot.ino"
uint32_t lerpColor(uint32_t from, uint32_t to, float t);
#line 459 "C:\\rust\\face_rbot\\face_rbot.ino"
void renderToScreen();
#line 657 "C:\\rust\\face_rbot\\face_rbot.ino"
void setup();
#line 689 "C:\\rust\\face_rbot\\face_rbot.ino"
void loop();
#line 118 "C:\\rust\\face_rbot\\face_rbot.ino"
int getStateIndex(int temp, int sound, int touch) {
  return temp * 4 + sound * 2 + touch;
}

// Hàm giả lập đọc cảm biến
void readMockSensors() {
  // Sinh ngẫu nhiên môi trường để AI có cái học
  if (random(0, 100) < 5) currentTemp = random(0, 3);
  if (random(0, 100) < 10) currentSound = random(0, 2);
  if (random(0, 100) < 10) currentTouch = random(0, 2);
}

// Hàm tính phần thưởng (Reward Function) - Định hình tính cách Robot
float calculateReward(int state, int action) {
  float reward = 0.0f;
  
  // Tách trạng thái ngược lại từ State Index
  int touch = state % 2;
  int sound = (state / 2) % 2;
  int temp = (state / 4) % 3;

  // Tính cách 1: Thích được vuốt ve (Touch -> Happy/Talk: +1)
  if (touch == TOUCHED) {
    if (action == 0 || action == 2) reward += 2.0f; // Happy / Talk
    else if (action == 4) reward -= 1.0f; // Nếu tức giận khi được vuốt -> Phạt
  }

  // Tính cách 2: Ghét ồn ào lúc ngủ (Noisy -> Sleep: Phạt nặng)
  if (sound == NOISY) {
    if (action == 3) reward -= 3.0f; // Đang ồn mà đi ngủ -> Phạt
    if (action == 4 || action == 5) reward += 1.0f; // Tức giận / Ngạc nhiên khi ồn -> Thưởng
  }

  // Tính cách 3: Nóng nảy (Hot -> Angry: +1)
  if (temp == HOT) {
    if (action == 4) reward += 1.5f; // Nóng thì dễ cáu
    if (action == 0) reward -= 1.0f; // Nóng mà vẫn Happy -> Hơi vô lý -> Phạt nhẹ
  }

  return reward;
}

// Thuật toán Bellman (Cập nhật Q-Table)
void learn(int state, int action, float reward, int nextState) {
  float maxFutureQ = qTable[nextState][0];
  for (int i = 1; i < NUM_ACTIONS; i++) {
    if (qTable[nextState][i] > maxFutureQ) {
      maxFutureQ = qTable[nextState][i];
    }
  }
  // Công thức cập nhật kinh nghiệm
  qTable[state][action] = qTable[state][action] + LEARNING_RATE * (reward + DISCOUNT_FACTOR * maxFutureQ - qTable[state][action]);
}

// Task chạy trên Core 0 (Độc lập với Vẽ đồ họa)
void AITask(void *pvParameters) {
  Serial.println("=========================================");
  Serial.println("AI DANG DUOC TAM DUNG DE DEBUG.");
  Serial.println("Vui long nhap so tu 0 den 16 de doi mat:");
  Serial.println("0:Idle 1:Normal 2:Happy 3:Sad 4:Talk 5:Sleep");
  Serial.println("6:Angry 7:Surprised 8:Doubt 9:Cry 10:Dizzy");
  Serial.println("11:Wink 12:LookAround 13:Panic 14:Smug");
  Serial.println("15:Scan 16:Bored");
  Serial.println("=========================================");

  // int currentState = getStateIndex(currentTemp, currentSound, currentTouch);

  for (;;) {
    if (Serial.available() > 0) {
      int code = Serial.parseInt();
      // Đọc bỏ các ký tự thừa (như \n, \r)
      while(Serial.available() > 0) Serial.read();

      if (code >= 0 && code < NUM_ACTIONS) {
        targetEmotionCode = code;
        Serial.print(">> Chuyen sang trang thai: ");
        Serial.println(code);
      } else {
        Serial.println("Loi: Ma cam xuc phai tu 0 den 12.");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Quét Serial mỗi 100ms
  }
}
// ==========================================

float lerpSpeed = 0.3; 

// --- OVERRIDE: BLINK MANAGER ---
float blinkFactor = 1.0;
float targetBlinkFactor = 1.0;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkDelay = 3000;
int sleepBlinkCount = 0; // Đếm số lần chớp mắt lúc buồn ngủ

void updateFaceLogic() {
  // Điều chỉnh tốc độ chuyển trạng thái (Animation Timing Tùy chỉnh)
  float currentLerp = 0.1; // Default
  if (targetFace.eyeHeight == stateSleep.eyeHeight) {
    currentLerp = 0.25; // Ngủ: Nhanh hơn bình thường một chút
  }
  else if (targetFace.eyeHeight == stateSurprised.eyeHeight) {
    currentLerp = 0.6;  // Ngạc nhiên: Giật bắn mình mở to mắt (Cực nhanh)
  }
  else if (targetFace.eyeAngle == stateAngry.eyeAngle) {
    currentLerp = 0.4;  // Giận dữ: Quắc mắt dứt khoát
  }
  else if (targetFace.eyeHeight == stateDoubt.eyeHeight) {
    currentLerp = 0.25; // Nghi ngờ: Nhanh hơn (đã tăng tốc)
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
  }

  // Khóa chớp mắt tự động khi đang thực hiện Wink (11) để không bị trùng lặp, mất mượt mà
  if (targetEmotionCode == 11) {
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

  uint16_t c = tft.color565(r, g, b);
  return (uint32_t)((c >> 8) | (c << 8)); // Đảo byte (Endianness Swap) để chống lại lỗi hiển thị ngược của SPI LCD
}

// Thuật toán Scanline Rasterization vẽ bo góc Elip bất đối xứng + Gradient Dọc (VGradient) siêu mượt
void drawGradientAsymmetricRect(LGFX_Sprite* spr, float cx, float cy, float w, float h, float shapeType, uint32_t colorTop, uint32_t colorBot, bool isMouth, float pitchFactor = 0.0f) {
  float rTL_x=0, rTL_y=0, rTR_x=0, rTR_y=0, rBR_x=0, rBR_y=0, rBL_x=0, rBL_y=0;
  
  if (!isMouth) {
    int type = (int)(shapeType + 0.5);
    
    // Tính tỉ lệ nhắm mắt hiện tại
    float blinkRatio = h / (currentFace.eyeHeight > 0.1f ? currentFace.eyeHeight : 40.0f);
    if (blinkRatio > 1.0f) blinkRatio = 1.0f;
    
    // Thu nhỏ bán kính (Radius) cực nhanh khi nhắm mắt để tạo hiệu ứng mí mắt khép dẹt thành đường thẳng (không bo tròn thành viên thuốc)
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
    // Sử dụng thông số shapeType được truyền từ renderToScreen làm hệ số bo tròn
    float topRadiusFactor = shapeType; 
    
    // Tự động nội suy đáy: Khi góc trên mở tròn, góc dưới cũng thu tròn lại để thành hình chữ O
    float bottomRadiusFactor = 1.0f - topRadiusFactor;

    rTL_x = rTR_x = w * topRadiusFactor; rTL_y = rTR_y = h * topRadiusFactor;
    rBL_x = rBR_x = w * 0.5f;  rBL_y = rBR_y = h * bottomRadiusFactor;
  }

  // Chống tràn bán kính
  if (rTL_x > w/2) rTL_x = w/2; if (rTR_x > w/2) rTR_x = w/2;
  if (rBL_x > w/2) rBL_x = w/2; if (rBR_x > w/2) rBR_x = w/2;
  if (rTL_y > h/2) rTL_y = h/2; if (rTR_y > h/2) rTR_y = h/2;

  if (!isMouth) {
    // Với Mắt: Giới hạn bán kính góc dưới là h/2 để đối xứng với góc trên, tránh hiện tượng đè nét khi nháy mắt (h nhỏ)
    if (rBL_y > h/2) rBL_y = h/2; if (rBR_y > h/2) rBR_y = h/2; 
  } else {
    // Với Miệng: Cho phép bán kính góc dưới lấn hết toàn bộ chiều cao (h) để tạo thành hình U (bán nguyệt đáy)
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
    // Thu hẹp 2 cạnh bên theo góc ngước lên/cúi xuống để tạo chiều sâu không gian
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

void drawEye(float centerX, float centerY, bool isRightEye, float scale3D = 1.0f, float extraAngle = 0.0f, float customBlink = -1.0f, float pitchFactor = 0.0f) {
  eyeSprite.fillSprite(TFT_BLACK);
  float pivotX = 60, pivotY = 60;
  eyeSprite.setPivot(pivotX, pivotY);

  // ĐỊNH NGHĨA DẢI MÀU (Bảng màu tĩnh theo chuẩn RGB888 Hex)
  uint32_t colorTop    = 0x00DC00; // (0, 220, 0)
  uint32_t colorMid    = 0x00D700; // (0, 215, 0)
  uint32_t colorBot    = 0x00D200; // (0, 210, 0)
  uint32_t shadowColor = 0x00C800; // (0, 200, 0)

  float actualBlink = (customBlink >= 0.0f) ? customBlink : blinkFactor;
  float w = currentFace.eyeWidth * scale3D;
  float h = currentFace.eyeHeight * actualBlink * scale3D; // Áp dụng Blink Override & 3D Scale
  if (h < 4) h = 4; // Guardrail: tối thiểu 4 pixel để thuật toán vát góc không sập
  float shape = currentFace.eyeShapeType;

  // 1. Vẽ Bóng Giả (Dark Blue) làm nền
  // Thu hẹp bề ngang (w - 4) để bóng không bị bè ra 2 bên góc, làm kích thước bóng nhỏ lại
  drawGradientAsymmetricRect(&eyeSprite, pivotX, pivotY, w - 4, h, shape, shadowColor, shadowColor, false, pitchFactor);

  if (targetEmotionCode == 10) {
    // 2. Vẽ Hiệu ứng Xoáy Thôi Miên (Hypnotic Spirals)
    // Lấy màu Gốc từ colorTop, ép kiểu 16-bit và Swap-byte để chữa lỗi Endianness
    uint16_t c = tft.color565((colorTop >> 16) & 0xFF, (colorTop >> 8) & 0xFF, colorTop & 0xFF);
    uint32_t swappedColor = (c >> 8) | (c << 8);

    float spin = millis() * 0.3f; // Tốc độ xoay
    for (int r = 5; r < (w / 2) - 2; r += 6) {
      // Vẽ vòng cung đứt khúc, góc lệch nhau tạo hình xoáy
      eyeSprite.drawArc(pivotX, pivotY, r, r - 2, spin - r * 15, spin - r * 15 + 180, swappedColor);
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

  // Hiệu ứng Quét Radar (Scan - Code 15): Mắt trượt qua lại đều đặn
  if (targetEmotionCode == 15) {
    effX += sin(millis() / 300.0f) * 35.0f;
  }
  
  // Nhìn xung quanh (LookAround - Code 12) kiểu nhìn bất định (Random Wandering)
  static float lookAroundOffsetX = 0.0f;
  static float lookAroundOffsetY = 0.0f;
  static float targetLookX = 0.0f;
  static float targetLookY = 0.0f;
  static unsigned long lastLookTime = 0;

  if (targetEmotionCode == 12) {
    // Nếu quá thời gian, chọn một điểm nhìn mới ngẫu nhiên (từ 0.8s đến 2s)
    if (millis() - lastLookTime > random(800, 2000)) { 
      targetLookX = random(-35, 36); // Liếc qua lại trục X
      targetLookY = random(-20, 21); // Liếc lên xuống trục Y
      lastLookTime = millis();
    }
    // Tính toán khoảng cách nội suy
    float stepX = (targetLookX - lookAroundOffsetX) * 0.05f; 
    float stepY = (targetLookY - lookAroundOffsetY) * 0.05f; 
    
    // Giới hạn tốc độ liếc tối đa để mắt trôi đi mượt mà, không bị "giật cái" khi khoảng cách quá xa
    if (stepX > 1.2f) stepX = 1.2f; if (stepX < -1.2f) stepX = -1.2f;
    if (stepY > 1.2f) stepY = 1.2f; if (stepY < -1.2f) stepY = -1.2f;
    
    lookAroundOffsetX += stepX;
    lookAroundOffsetY += stepY;
  } else {
    // Khi thoát trạng thái, cũng từ từ trôi về giữa một cách êm ái
    float stepX = (0.0f - lookAroundOffsetX) * 0.05f;
    float stepY = (0.0f - lookAroundOffsetY) * 0.05f;
    if (stepX > 1.2f) stepX = 1.2f; if (stepX < -1.2f) stepX = -1.2f;
    if (stepY > 1.2f) stepY = 1.2f; if (stepY < -1.2f) stepY = -1.2f;
    lookAroundOffsetX += stepX; 
    lookAroundOffsetY += stepY;
  }
  effX += lookAroundOffsetX;
  effY += lookAroundOffsetY;

  // Hiệu ứng Nháy mắt (Wink - Code 11): Liếc mắt trái -> nháy mắt phải -> trả về
  static float winkOffset = 0.0f;
  float leftBlink = -1.0f;
  float rightBlink = -1.0f;

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
  float leftEyeScale = 1.0f + (effX * 0.002f); 
  float rightEyeScale = 1.0f - (effX * 0.002f);
  
  // Phối cảnh 3D hình thang (Pitch): Ngước lên (effY < 0) thì trên bé lại. Cúi xuống (effY > 0) thì dưới bé lại.
  float eyePitchFactor = -effY * 0.015f; 

  drawEye(eyeLx, eyeY, false, leftEyeScale, 0.0f, leftBlink, eyePitchFactor); 
  drawEye(eyeRx, eyeY, true, rightEyeScale, 0.0f, rightBlink, eyePitchFactor);

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
    // Chỉ kích hoạt khi mục tiêu CHÍNH XÁC là trạng thái Nói (chiều cao miệng == 35)
    // Điều này tránh việc trạng thái Ngạc nhiên (chiều cao 40) bị mấp máy môi sai cách
    if (abs(targetFace.mouthHeight - 35.0f) < 0.1f) {
      // Giảm tốc độ nói (chia cho 150.0f thay vì 80.0f) để khớp với nhịp điệu tự nhiên hơn
      float talkPhase = millis() / 150.0f;
      float talkFactor = 0.3f + 0.7f * abs(sin(talkPhase) * sin(talkPhase * 0.6f));
      h = h * talkFactor;
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

    // Cả miệng cũng chịu hiệu ứng Trapezoid Pitch như mắt để đồng bộ (cùng nghiêng mặt)
    float mouthPitchFactor = -effY * 0.015f;

    // Truyền topRadiusFactor vào tham số thứ 6 (shapeType) để đảm bảo mọi lớp đồng bộ độ bo góc
    drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY, w - 4, h, topRadiusFactor, shadowColor, shadowColor, true, mouthPitchFactor);
    
    // Chuyển màu phân lớp cho Miệng (viền mỏng hơn)
    drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY - 1, w,     h,     topRadiusFactor, colorBot, colorBot, true, mouthPitchFactor);
    if (h > 2) drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY - 2, w - 2, h - 2, topRadiusFactor, colorMid, colorMid, true, mouthPitchFactor);
    if (h > 4) drawGradientAsymmetricRect(&canvasSprite, mouthX, mouthY - 3, w - 4, h - 4, topRadiusFactor, colorTop, colorTop, true, mouthPitchFactor);
  }

  canvasSprite.pushSprite(0, 0);
}

unsigned long lastUpdate = 0;
unsigned long stateChangeTimer = 0;
unsigned long nextStateDelay = 2000;

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(0); 
  tft.fillScreen(tft.color565(0, 0, 0)); 

  canvasSprite.setPsram(true); 
  canvasSprite.setColorDepth(16); 
  eyeSprite.setPsram(true);
  eyeSprite.setColorDepth(16);

  if (canvasSprite.createSprite(240, 240) == nullptr) {
    Serial.println("LỖI: Không thể tạo Canvas Sprite!");
    while (1); 
  }
  if (eyeSprite.createSprite(120, 120) == nullptr) {
    Serial.println("LỖI: Không thể tạo Eye Sprite!");
    while (1); 
  }

  // Khởi chạy AI Task trên Core 0 (Priority 1)
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
  // Nhận biết sự thay đổi cảm xúc từ AI Task
  if (targetEmotionCode != lastEmotionCode) {
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
    case 5: 
      // Đợi 2500ms (cho nhịp nháy mắt lờ đờ diễn ra trọn vẹn) rồi mới gán stateSleep để gục hẳn
      if (millis() - sleepStartTime > 2500) {
        targetFace = stateSleep; 
      } else {
        targetFace = stateNormal; 
        targetFace.eyeAngle = 0; // Quan trọng: Ép mắt phẳng ngang lờ đờ để mí sụp không bị tạo hình chữ V (giận dữ)
        targetFace.offsetY = 8;  // Đầu đã bắt đầu cúi gục nhẹ
      }
      break;
    case 6: targetFace = stateAngry; break;
    case 7: targetFace = stateSurprised; break;
    case 8: targetFace = stateDoubt; break;
    case 9: targetFace = stateCry; break;
    case 10: targetFace = stateDizzy; break;
    case 11: targetFace = stateNormal; break; // Nháy mắt như khuôn mặt 1 (Normal)
    case 12: break; // LookAround -> KHÔNG gán targetFace, giữ nguyên biểu cảm miệng hiện tại
    case 13: targetFace = statePanic; break;
    case 14: targetFace = stateSmug; break;
    case 15: targetFace = stateScan; break;
    case 16: targetFace = stateBored; break;
    default: targetFace = stateIdle; break;
  }

  // Cập nhật Logic & Render liên tục trên Core 1
  updateFaceLogic();
  renderToScreen();

  // Nhường CPU cho FreeRTOS (Delay 50ms = 20 FPS ổn định)
  vTaskDelay(pdMS_TO_TICKS(50));
}




