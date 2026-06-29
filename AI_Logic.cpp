#include "AI_Logic.h"
const int NUM_STATES = 3 * 2 * 2; // 12 Trạng thái
const int NUM_ACTIONS = 22; // 22 Biểu cảm (Thêm 21: Petting)

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
