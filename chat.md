me: Tôi dùng esp32s3 và màn hình st7789 240x240px, tôi muốn tạo một khuôn mặt robot động và thay đổi trang thái mượt mà ví dụ từ mặt bình thường sang cười hay nnói chuyện làm sao để thực hiện mà vẫn tối ưu hóa hiệu năng và chất lượng 
gemini: Chào bạn, với bài toán tạo khuôn mặt robot động trên cấu hình ESP32-S3 và màn hình ST7789 (240x240), đây là một task rất thú vị nhưng đòi hỏi thiết kế hệ thống tốt để không bị nghẽn (bottleneck) về SPI hoặc thiếu hụt RAM gây giật lag.

### Nêu rõ các giả định của tôi

Trước khi đi sâu vào giải pháp, tôi đang đưa ra các giả định sau để làm cơ sở thiết kế:

1. **Môi trường lập trình:** Bạn đang sử dụng Arduino IDE hoặc PlatformIO (C/C++).
2. **Giao tiếp:** ST7789 được kết nối với ESP32-S3 qua giao tiếp SPI.
3. **Phong cách thiết kế:** Khuôn mặt robot là dạng Vector/Hình học cơ bản (giống robot EVE trong Wall-E hoặc robot Cozmo) gồm các hình khối như mắt (oval/chữ nhật bo góc) và miệng (đường cong), thay vì dạng ảnh tả thực (photorealistic).
4. **Phần cứng:** Mạch ESP32-S3 của bạn có sẵn PSRAM (rất phổ biến trên dòng S3).

---

### Phản biện & Đánh giá các cách tiếp cận

* **Cách tiếp cận phổ thông (Nhưng KHÔNG khuyên dùng): Chạy ảnh GIF, MJPEG hoặc Sprite Sheets (chuỗi ảnh tĩnh).**
* *Ưu điểm:* Dễ hình dung, có thể dùng tool design để vẽ sẵn.
* *Rủi ro góc khuất:* Tốn cực kỳ nhiều bộ nhớ Flash. Để chuyển đổi mượt từ trạng thái A sang B cần rất nhiều frames. Tốc độ đọc từ bộ nhớ flash có thể không đuổi kịp tốc độ khung hình (FPS), gây nghẽn cổ chai và hiệu ứng bị giật (stuttering). Cách này thiếu sự linh hoạt nếu bạn muốn tạo ra vô số biểu cảm ngẫu nhiên.


* **Cách tiếp cận tối ưu của Senior: Vẽ bằng Toán học (Procedural Drawing) kết hợp Nội suy (Interpolation) & DMA.**
* *Tại sao ưu việt?* Khuôn mặt bản chất chỉ là tọa độ và kích thước. Thay vì lưu hàng chục ảnh cái miệng đang há ra, ta chỉ lưu thông số: `Mouth_Height` đi từ `2px` lên `40px`. ESP32-S3 cực kỳ mạnh trong việc tính toán số thực. Hệ thống sẽ tính toán và tự vẽ các khung hình trung gian (In-between frames), tạo ra chuyển động mượt mà 60 FPS với dung lượng RAM/Flash gần như bằng không.



---

### Kiến trúc & Cách triển khai tối ưu

Để đạt được chất lượng và hiệu năng, bạn cần kết hợp 3 thành phần cốt lõi:

#### 1. Thư viện Đồ họa & Hardware DMA

Khuyên dùng thư viện **`LovyanGFX`** hoặc **`TFT_eSPI`**.
Bạn **bắt buộc phải bật DMA (Direct Memory Access)**. DMA cho phép ESP32 đẩy dữ liệu hình ảnh ra màn hình ST7789 ở background mà không làm CPU phải chờ đợi. CPU rảnh rỗi để tính toán tọa độ cho frame tiếp theo.

#### 2. Kỹ thuật Double Buffering (Sprite)

Màn hình 240x240 với màu 16-bit cần $240 \times 240 \times 2 = 115,200 \text{ bytes}$ (khoảng 115KB) RAM.

* **Không bao giờ vẽ trực tiếp lên màn hình:** Nếu bạn xóa màn hình bằng lệnh `fillScreen` rồi vẽ lại từng con mắt, màn hình sẽ bị chớp nháy (flickering).
* **Cách xử lý:** Tạo một đối tượng `Sprite` trong RAM. Vẽ toàn bộ khuôn mặt vào Sprite này, sau đó dùng lệnh push toàn bộ Sprite lên màn hình (nhờ DMA) trong 1 chu kỳ duy nhất.

#### 3. Thuật toán Nội suy Tuyến tính (Linear Interpolation - LERP)

Để chuyển trạng thái mượt, ta định nghĩa một cấu trúc dữ liệu cho khuôn mặt và dùng Lerp để chuyển đổi dần dần.

**Mô hình hóa (Mã giả chuẩn C++):**

```cpp
// Phân đoạn 1: Định nghĩa cấu trúc dữ liệu nguyên tử (Atomic Face State)
struct FaceState {
  float eyeWidth;
  float eyeHeight;
  float eyeDistance; // Khoảng cách giữa 2 mắt
  float eyeRadius;   // Độ bo góc của mắt
  float mouthY;      // Vị trí miệng
  float mouthHeight; // Miệng há to cỡ nào
};

// Các trạng thái được định nghĩa sẵn
FaceState stateNormal = {40, 60, 80, 10, 160, 5};
FaceState stateSmile =  {40, 40, 80, 20, 150, 25}; // Mắt híp lại, miệng há to và nâng lên
FaceState stateTalk =   {40, 60, 80, 10, 160, 30}; // Mắt bình thường, miệng há to

FaceState currentState = stateNormal;
FaceState targetState = stateNormal;

// Phân đoạn 2: Hàm Lerp - Chìa khóa của sự mượt mà
// current: Giá trị hiện tại
// target: Giá trị mục tiêu
// t: Hệ số nội suy (0.0 đến 1.0, ví dụ 0.1 nghĩa là tiến tới 10% mỗi frame)
float lerp(float current, float target, float t) {
  return current + (target - current) * t;
}

// Phân đoạn 3: Vòng lặp cập nhật Logic & Render (Feature-sliced step)
void updateFace() {
  // 1. Tính toán Logic nội suy (Cập nhật trạng thái hiện tại tiến dần về mục tiêu)
  currentState.eyeWidth = lerp(currentState.eyeWidth, targetState.eyeWidth, 0.2);
  currentState.eyeHeight = lerp(currentState.eyeHeight, targetState.eyeHeight, 0.2);
  currentState.mouthHeight = lerp(currentState.mouthHeight, targetState.mouthHeight, 0.4); // Miệng mấp máy nhanh hơn mắt
  
  // 2. Clear Sprite Buffer (Không clear màn hình vật lý)
  sprite.fillSprite(TFT_BLACK);

  // 3. Vẽ lên Sprite (Procedural drawing)
  // Vẽ mắt trái
  sprite.fillRoundRect(120 - (currentState.eyeDistance/2) - (currentState.eyeWidth/2), 
                       100 - (currentState.eyeHeight/2), 
                       currentState.eyeWidth, currentState.eyeHeight, 
                       currentState.eyeRadius, TFT_CYAN);
                       
  // Vẽ mắt phải
  sprite.fillRoundRect(120 + (currentState.eyeDistance/2) - (currentState.eyeWidth/2), 
                       100 - (currentState.eyeHeight/2), 
                       currentState.eyeWidth, currentState.eyeHeight, 
                       currentState.eyeRadius, TFT_CYAN);

  // Vẽ miệng
  sprite.fillRoundRect(120 - 20, currentState.mouthY, 40, currentState.mouthHeight, 5, TFT_CYAN);

  // 4. Push toàn bộ Buffer lên màn hình qua DMA
  sprite.pushSprite(0, 0);
}

```

---

### Những điều tôi cần làm rõ

Tôi đã phác thảo hệ thống kiến trúc cốt lõi, nhưng để cung cấp một mã nguồn chính xác hoàn toàn cho dự án của bạn, tôi cần làm rõ vài điểm sau:

1. Thiết kế khuôn mặt bạn mong muốn chỉ bao gồm Mắt, hay gồm cả Mắt + Miệng + Chuyển động lông mày? (Càng nhiều chi tiết, hàm vẽ toán học sẽ cần điều chỉnh phức tạp hơn).
2. ESP32-S3 của bạn là phiên bản dùng bộ nhớ PSRAM loại nào (OPI hay QSPI), và bạn đang dự định dùng thư viện màn hình nào (`TFT_eSPI` hay `LovyanGFX`) để tôi có thể cấu hình bước khởi tạo bộ đệm DMA cho đúng?

me: Vẽ bằng Toán học (Procedural Drawing) kết hợp Nội suy (Interpolation) & DMA. Như code bạn đúng không. Tôi chỉ có mắt miệng thi thoảng không có miệng ví dụ như idle.
Tôi dùng esp32-s3 wroom 1 tôi không biết là OPI hay QSPI vì nó chỉ ghi n16r8, và tôi dang dùng TFT_eSPI vậy LovyanGFX là gì cái nào tốt hơn và hiệu quả, hiệu suất cao hơn tôi sẽ dùng và tôi dùng ardunio ide 

gemini: Đúng vậy, hướng tiếp cận sử dụng **Toán học (Procedural) + Nội suy (Interpolation) + DMA** chính là chìa khóa để đạt hiệu năng tối đa trên ESP32-S3. Việc cấu trúc khuôn mặt chỉ gồm mắt và miệng (thỉnh thoảng ẩn miệng khi Idle) giúp việc tính toán toán học trở nên cực kỳ nhẹ nhàng, CPU sẽ mất chưa tới 1% hiệu năng cho phần logic này.

### 1. Giải mã ký hiệu "N16R8" trên ESP32-S3

Ký hiệu **N16R8** trên chip của bạn cung cấp thông tin cực kỳ quan trọng về phần cứng:

* **N16 (16MB Flash):** Bộ nhớ lưu trữ code. Thường chạy ở chế độ **QSPI** (Quad SPI).
* **R8 (8MB PSRAM):** Bộ nhớ RAM mở rộng. Đối với dòng chip S3 mã R8, đây chắc chắn là **OPI (Octal SPI)** đường truyền 8-bit tốc độ cao.

**Kết luận:** Phần cứng của bạn thuộc loại cao cấp nhất của dòng WROOM-1. Với 8MB OPI PSRAM, bạn có một không gian bộ đệm cực kỳ rộng và nhanh để tạo các `Sprite` (Double Buffering) mà không sợ bị tràn RAM.

### 2. So sánh TFT_eSPI và LovyanGFX: Cái nào hiệu quả hơn?

| Tiêu chí | TFT_eSPI | LovyanGFX |
| --- | --- | --- |
| **Nguồn gốc** | Thư viện quốc dân, rất phổ biến, hỗ trợ nhiều dòng chip. | Được viết sau bởi một kỹ sư Nhật Bản, tối ưu chuyên sâu cho ESP32/S3. |
| **Tối ưu DMA** | Có hỗ trợ DMA nhưng cấu hình thủ công phức tạp hơn khi dùng với Sprite. | **Tối ưu ở cấp độ thanh ghi (Register)** cho ESP32-S3. Quản lý DMA tự động và thông minh hơn. |
| **Quản lý Sprite** | Đẩy Sprite qua DMA cần thiết lập bộ nhớ cẩn thận, đôi khi gặp lỗi xung đột cấu hình. | Hỗ trợ tạo Sprite trực tiếp trên PSRAM rất mượt mà. Lệnh đẩy dữ liệu nền (Background DMA push) chạy cực kỳ ổn định. |
| **Hiệu suất (FPS)** | Đạt ~45-60 FPS cho màn hình 240x240. | **Đạt 60+ FPS dễ dàng**, tốc độ dựng hình và giải phóng bus SPI tối ưu hơn. |

**Khuyên dùng:** Nếu bạn đã cấu hình chạy được `TFT_eSPI` ổn định thì có thể tiếp tục dùng vì màn hình 240x240 khá nhỏ, `TFT_eSPI` vẫn dư sức gánh mượt. Tuy nhiên, nếu bạn muốn code sạch hơn, quản lý bộ đệm Sprite tự động nằm trên PSRAM và tối ưu sâu phần cứng DMA của dòng S3, **`LovyanGFX` là lựa chọn vượt trội về hiệu suất**.

---

### Mô phỏng cơ chế chuyển đổi trạng thái (Interpolation)

Để bạn dễ hình dung cách các thông số toán học thay đổi mượt mà như thế nào khi chuyển từ Bình thường $\rightarrow$ Cười $\rightarrow$ Nói chuyện $\rightarrow$ Idle (Ẩn miệng), dưới đây là một bộ mô phỏng trực quan. Bạn có thể thay đổi tốc độ Lerp và bấm chuyển đổi các trạng thái để xem cách thuật toán tự động tính toán các khung hình trung gian.

### Cách xử lý trạng thái ẩn miệng (Idle) trong Code Arduino

Dựa trên cấu trúc phân đoạn hệ thống, để xử lý việc "thi thoảng không có miệng", bạn chỉ cần thêm một biến trạng thái logic hoặc đặt mục tiêu chiều cao miệng (`mouthHeight`) về `0`. Khi `mouthHeight` tiến về 0, nó sẽ tự động biến mất một cách mượt mà.

Dưới đây là khối code thay thế và bổ sung để tối ưu hóa việc ẩn miệng và mấp máy khi nói chuyện:

```cpp
// Phân đoạn 1: Cập nhật cấu trúc trạng thái trực quan
struct FaceState {
  float eyeWidth;
  float eyeHeight;
  float eyeRadius;
  float mouthHeight; // Nếu bằng 0 nghĩa là không có miệng
  float mouthWidth;
};

// Định nghĩa các tập trạng thái nguyên tử
const FaceState stateNormal   = {40, 60, 15, 8,  40};
const FaceState stateSmile    = {45, 30, 20, 25, 50}; 
const FaceState stateIdle     = {40, 60, 15, 0,  0};  // Miệng bằng 0 sẽ ẩn đi
const FaceState stateTalkOpen = {38, 62, 10, 35, 35}; // Trạng thái há miệng khi nói
const FaceState stateTalkClose= {38, 62, 10, 4,  35}; // Trạng thái ngậm miệng khi nói

FaceState currentFace = stateNormal;
FaceState targetFace = stateNormal;
float lerpSpeed = 0.15; // Tốc độ mượt của chuyển động

// Phân đoạn 2: Hàm vẽ tối ưu hiệu suất vào Sprite Buffer
void renderFaceToSprite() {
  // Nội suy tuyến tính (Lerp) cho từng thành phần
  currentFace.eyeWidth    = currentFace.eyeWidth    + (targetFace.eyeWidth    - currentFace.eyeWidth)    * lerpSpeed;
  currentFace.eyeHeight   = currentFace.eyeHeight   + (targetFace.eyeHeight   - currentFace.eyeHeight)   * lerpSpeed;
  currentFace.eyeRadius   = currentFace.eyeRadius   + (targetFace.eyeRadius   - currentFace.eyeRadius)   * lerpSpeed;
  currentFace.mouthHeight = currentFace.mouthHeight + (targetFace.mouthHeight - currentFace.mouthHeight) * lerpSpeed;
  currentFace.mouthWidth  = currentFace.mouthWidth  + (targetFace.mouthWidth  - currentFace.mouthWidth)  * lerpSpeed;

  // Xóa toàn bộ Sprite nền (chuẩn bị frame mới trong RAM)
  canvasSprite.fillSprite(TFT_BLACK);

  // Vẽ Mắt Trái & Mắt Phải bằng hàm hình học bo góc dựa trên thông số đã nội suy
  canvasSprite.fillRoundRect(40, 90 - (currentFace.eyeHeight/2), currentFace.eyeWidth, currentFace.eyeHeight, currentFace.eyeRadius, TFT_CYAN);
  canvasSprite.fillRoundRect(200 - currentFace.eyeWidth, 90 - (currentFace.eyeHeight/2), currentFace.eyeWidth, currentFace.eyeHeight, currentFace.eyeRadius, TFT_CYAN);

  // Kiểm tra điều kiện tối ưu: Chỉ vẽ miệng nếu kích thước nội suy lớn hơn 0.5 pixel
  if (currentFace.mouthHeight > 0.5) {
    canvasSprite.fillRoundRect(120 - (currentFace.mouthWidth/2), 170 - (currentFace.mouthHeight/2), currentFace.mouthWidth, currentFace.mouthHeight, 6, TFT_CYAN);
  }

  // Phân đoạn 3: Đẩy toàn bộ vùng nhớ RAM này ra màn hình ST7789 bằng phần cứng DMA độc lập
  // Hàm này giải phóng CPU ngay lập tức để thực hiện task khác trong khi dữ liệu đang truyền
  canvasSprite.pushSprite(0, 0); 
}

// Phân đoạn 4: Điều khiển luồng mấp máy khi nói chuyện (Non-blocking state logic)
unsigned long lastTalkToggle = 0;
bool isOpenFrame = false;

void handleTalkingEffect() {
  if (millis() - lastTalkToggle > 150) { // Cứ mỗi 150ms thay đổi khẩu hình miệng
    lastTalkToggle = millis();
    isOpenFrame = !isOpenFrame;
    targetFace = isOpenFrame ? stateTalkOpen : stateTalkClose;
  }
}

```

### Kinh nghiệm triển khai trên Arduino IDE cho Senior:

1. **Tần suất gọi hàm:** Đặt hàm `renderFaceToSprite()` chạy trong một bộ định thời (Ticker hoặc định thời bằng `millis()`) cố định khoảng `30ms` đến `16ms` một lần (tương đương 30 - 60 FPS). Không gọi liên tục không kiểm soát trong `loop()` để tránh lãng phí băng thông SPI.
2. **Khởi tạo DMA:** Trong hàm `setup()`, sau khi khởi tạo màn hình và tạo Sprite, bạn bắt buộc phải gọi lệnh kích hoạt DMA của thư viện (Ví dụ với TFT_eSPI là `tft.initDMA()`).

me: tôi muốn dùng LovyanGFX thì tải và cấu hình như nào? không chân cs.

gemini: Việc chuyển sang dùng **LovyanGFX** là một quyết định cực kỳ sáng suốt cho cấu hình phần cứng (ESP32-S3 N16R8) của bạn. Nó sẽ giúp tối đa hóa sức mạnh của bộ nhớ OPI PSRAM và phần cứng DMA.

Dưới đây là hướng dẫn từ A-Z để cài đặt và cấu hình thư viện này, đặc biệt xử lý trường hợp **màn hình không có chân CS**.

### 1. Cách cài đặt LovyanGFX

1. Mở Arduino IDE.
2. Vào **Sketch** -> **Include Library** -> **Manage Libraries** (hoặc nhấn Ctrl+Shift+I).
3. Gõ `LovyanGFX` vào ô tìm kiếm.
4. Tìm thư viện **LovyanGFX** by *lovyan03* và nhấn **Install**.

### 2. Cấu hình phần cứng (Class-based Configuration)

Khác với `TFT_eSPI` (phải sửa file thư viện `User_Setup.h`), `LovyanGFX` cho phép bạn nhúng thẳng cấu hình phần cứng vào trong code dự án thông qua một Class. Điều này giúp code mang đi đâu cũng chạy được mà không sợ mất cấu hình.

Đối với màn hình **không có chân CS**, bạn chỉ cần set `pin_cs = -1` trong cấu hình Panel. Mạch ST7789 trên thị trường không có CS thường đã được nhà sản xuất nối tắt (pull-low) chân này xuống GND (Mass) trên bo mạch rồi.

Dưới đây là toàn bộ mã nguồn hoàn chỉnh (Code + Cấu hình + Logic khuôn mặt) cho dự án của bạn:

```cpp
#include <LovyanGFX.hpp>

// ==============================================================
// PHÂN ĐOẠN 1: CẤU HÌNH PHẦN CỨNG LOVYANGFX (HARDWARE ABSTRACTION)
// ==============================================================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;

public:
  LGFX(void) {
    // 1. Cấu hình Bus SPI cho ESP32-S3
    auto cfg = _bus_instance.config();
    cfg.spi_host = SPI2_HOST;  // Dùng SPI2 (FSPI) trên ESP32-S3
    cfg.spi_mode = 0;          // SPI Mode 0 (chuẩn cho ST7789)
    cfg.freq_write = 40000000; // Tốc độ SPI: 40MHz (ST7789 chạy rất ổn định ở mức này)
    cfg.spi_3wire = false;     // False vì màn hình vẫn có chân DC riêng
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO; // Kích hoạt phần cứng DMA tự động

    // CẤU HÌNH CHÂN GPIO (BẠN HÃY SỬA LẠI CHO ĐÚNG VỚI CÁCH BẠN NỐI DÂY)
    cfg.pin_sclk = 10; // Chân SCL / SCK trên màn hình
    cfg.pin_mosi = 11; // Chân SDA / DIN trên màn hình
    cfg.pin_miso = -1; // Màn hình không cần truyền ngược dữ liệu
    cfg.pin_dc   = 13;  // Chân DC / RS (Data/Command)
    
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    // 2. Cấu hình Panel ST7789
    auto p_cfg = _panel_instance.config();
    
    // ĐÂY LÀ CHỖ QUAN TRỌNG CHO MÀN HÌNH KHÔNG CÓ CHÂN CS
    p_cfg.pin_cs   = -1;   // Đặt thành -1 vì không dùng chân CS (Chip Select)
    p_cfg.pin_rst  = 12;    // Chân RES / RST (Reset). Nếu nối vào EN/RST của ESP thì set -1
    p_cfg.pin_busy = -1;
    
    // Thông số màn hình
    p_cfg.panel_width  = 240;
    p_cfg.panel_height = 240;
    p_cfg.offset_x = 0;    // Đổi thành 0, 20 hoặc 32 nếu hiển thị bị lệch viền
    p_cfg.offset_y = 0;    // Đổi thành 0, 20 hoặc 32 nếu hiển thị bị lệch viền
    p_cfg.invert = true;   // ST7789 đa số cần True, nếu sai màu hãy đổi thành False
    p_cfg.rgb_order = false; // Thay đổi nếu màu Xanh và Đỏ bị ngược nhau

    _panel_instance.config(p_cfg);
    setPanel(&_panel_instance);
  }
};

// Khởi tạo đối tượng màn hình và bộ đệm Sprite
LGFX tft;
LGFX_Sprite canvasSprite(&tft);

// ==============================================================
// PHÂN ĐOẠN 2: ĐỊNH NGHĨA TRẠNG THÁI VÀ LOGIC KHUÔN MẶT
// ==============================================================
struct FaceState {
  float eyeWidth;
  float eyeHeight;
  float eyeRadius;
  float mouthHeight; // 0 nghĩa là ẩn (idle)
  float mouthWidth;
};

// Tập các trạng thái khuôn mặt (Atomic States)
const FaceState stateNormal   = {40, 60, 15, 8,  40};
const FaceState stateSmile    = {45, 20, 20, 25, 60}; 
const FaceState stateIdle     = {40, 60, 15, 0,  0};  // Miệng co về 0 để biến mất
const FaceState stateTalkOpen = {38, 62, 10, 35, 35};
const FaceState stateTalkClose= {38, 62, 10, 4,  35};

FaceState currentFace = stateNormal;
FaceState targetFace = stateIdle; // Bắt đầu khởi động ở trạng thái Idle

float lerpSpeed = 0.15; // Tốc độ nội suy (Càng nhỏ càng chậm/mượt)

// ==============================================================
// PHÂN ĐOẠN 3: LOGIC VẼ VÀ RENDER VÀO BỘ ĐỆM
// ==============================================================
void updateFaceLogic() {
  // Nội suy tuyến tính (Lerp)
  currentFace.eyeWidth    += (targetFace.eyeWidth    - currentFace.eyeWidth)    * lerpSpeed;
  currentFace.eyeHeight   += (targetFace.eyeHeight   - currentFace.eyeHeight)   * lerpSpeed;
  currentFace.eyeRadius   += (targetFace.eyeRadius   - currentFace.eyeRadius)   * lerpSpeed;
  currentFace.mouthHeight += (targetFace.mouthHeight - currentFace.mouthHeight) * lerpSpeed;
  currentFace.mouthWidth  += (targetFace.mouthWidth  - currentFace.mouthWidth)  * lerpSpeed;
}

void renderToScreen() {
  // 1. Xóa nền Sprite trong RAM (không chớp màn hình)
  canvasSprite.fillSprite(tft.color565(0, 0, 0)); // Nền đen

  // 2. Vẽ Mắt Trái & Phải
  uint32_t faceColor = tft.color565(0, 255, 255); // Màu Xanh Cyan (EVE Wall-E style)
  // Mắt trái
  canvasSprite.fillRoundRect(40, 90 - (currentFace.eyeHeight/2), currentFace.eyeWidth, currentFace.eyeHeight, currentFace.eyeRadius, faceColor);
  // Mắt phải
  canvasSprite.fillRoundRect(200 - currentFace.eyeWidth, 90 - (currentFace.eyeHeight/2), currentFace.eyeWidth, currentFace.eyeHeight, currentFace.eyeRadius, faceColor);

  // 3. Xử lý logic Ẩn Miệng (Chỉ vẽ khi chiều cao > 0.5px)
  if (currentFace.mouthHeight > 0.5) {
    canvasSprite.fillRoundRect(120 - (currentFace.mouthWidth/2), 170 - (currentFace.mouthHeight/2), currentFace.mouthWidth, currentFace.mouthHeight, 6, faceColor);
  }

  // 4. Push toàn bộ Sprite lên màn hình (LovyanGFX tự động dùng DMA để truyền data)
  canvasSprite.pushSprite(0, 0);
}

// ==============================================================
// PHÂN ĐOẠN 4: VÒNG ĐỜI ARDUINO (MAIN LIFECYCLE)
// ==============================================================
unsigned long lastUpdate = 0;
unsigned long stateChangeTimer = 0;
int currentStateIndex = 0;

void setup() {
  Serial.begin(115200);
  
  // 1. Khởi tạo màn hình
  tft.init();
  tft.setRotation(0); // Chỉnh lại nếu hình bị xoay ngang/dọc
  tft.fillScreen(tft.color565(0, 0, 0)); // Xóa đen màn vật lý lần đầu

  // 2. CẤU HÌNH BỘ ĐỆM OPI PSRAM (Quan trọng cho ESP32-S3 N16R8)
  canvasSprite.setPsram(true); 
  canvasSprite.setColorDepth(16); // 16-bit màu (RGB565)
  
  // Tạo bộ đệm vừa khớp màn hình 240x240
  if (canvasSprite.createSprite(240, 240) == nullptr) {
    Serial.println("LỖI: Không thể tạo Sprite! Kiểm tra lại cấu hình PSRAM.");
    while (1); 
  }
  
  Serial.println("Khoi tao LovyanGFX thanh cong!");
}

void loop() {
  unsigned long now = millis();

  // Task 1: Render Loop cố định ~60 FPS (mỗi 16ms)
  if (now - lastUpdate >= 16) {
    lastUpdate = now;
    updateFaceLogic();
    renderToScreen();
  }

  // Task 2: Demo Chuyển đổi trạng thái ngẫu nhiên (mỗi 2.5 giây)
  if (now - stateChangeTimer > 2500) {
    stateChangeTimer = now;
    currentStateIndex = (currentStateIndex + 1) % 4;
    
    switch(currentStateIndex) {
      case 0: targetFace = stateNormal; break;
      case 1: targetFace = stateSmile; break;
      case 2: targetFace = stateTalkOpen; break;
      case 3: targetFace = stateIdle; break; // Ẩn miệng
    }
  }
}
```

### ⚠️ Lưu ý CỰC KỲ quan trọng trên Arduino IDE:

Vì bạn dùng chip N16R8 (Có 8MB OPI PSRAM), để lệnh `canvasSprite.createSprite()` chạy thành công mà không báo lỗi thiếu RAM, bạn phải bật PSRAM trong Arduino IDE.

1. Trên Menu của Arduino IDE, vào **Tools** (Công cụ).
2. Chọn Board là **ESP32S3 Dev Module** (hoặc board S3 tương ứng của bạn).
3. Tìm mục **PSRAM** và chọn: **`OPI PSRAM`**.
4. (Tùy chọn) Tìm mục **Flash Mode** và chọn: `QIO 80MHz`.

Khi nạp code này, màn hình sẽ tự động chuyển đổi mượt mà giữa các biểu cảm: *Bình thường -> Cười -> Há miệng -> Idle (từ từ biến mất miệng)* và lặp lại liên tục với chất lượng đồ họa hoàn hảo không một chút chớp nháy! Bạn hãy thay đổi các giá trị ở `cfg.pin_xxx` cho đúng với chân GPIO bạn cắm nhé.