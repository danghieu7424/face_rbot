#ifndef AI_LOGIC_H
#define AI_LOGIC_H

#include <Arduino.h>

// 1. Không gian trạng thái rời rạc (Quantized State Space)
enum TempState { COLD = 0, NORMAL = 1, HOT = 2 };
enum SoundState { QUIET = 0, NOISY = 1 };
enum TouchState { UNTOUCHED = 0, TOUCHED = 1 };

extern const int NUM_STATES;
extern const int NUM_ACTIONS;

// 2. Q-Table (Bộ nhớ Kinh nghiệm)
extern float qTable[12][22]; 

// 3. Siêu tham số Học tăng cường
extern const float LEARNING_RATE;
extern const float DISCOUNT_FACTOR;
extern float explorationRate;

// 4. Biến môi trường giả lập (Mock Sensors)
extern int currentTemp;
extern int currentSound;
extern int currentTouch;

// Hàm chuyển đổi tổ hợp cảm biến thành 1 mã trạng thái (0-11)
int getStateIndex(int temp, int sound, int touch);

// Hàm giả lập đọc cảm biến
void readMockSensors();

// Hàm tính phần thưởng (Reward Function) - Định hình tính cách Robot
float calculateReward(int state, int action);

// Thuật toán Bellman (Cập nhật Q-Table)
void learn(int state, int action, float reward, int nextState);

#endif
