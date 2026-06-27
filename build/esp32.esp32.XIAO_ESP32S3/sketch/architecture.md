#line 1 "C:\\rust\\face_rbot\\architecture.md"
# TÀI LIỆU KIẾN TRÚC HỆ THỐNG - FACE ROBOT ESP32-S3
*(Tài liệu này đóng vai trò như một Single Source of Truth cho toàn bộ dự án)*

## 1. TỔNG QUAN KIẾN TRÚC (HYBRID FSD + ATOMIC)
Dự án được xây dựng dựa trên nguyên lý chia rẽ mối bận tâm (Separation of Concerns) để đảm bảo ESP32-S3 hoạt động ở hiệu suất tối đa (20+ FPS) trong khi vẫn chạy được thuật toán Trí Tuệ Nhân Tạo (AI).

Hệ thống được chia làm 2 phân vùng (Cores) độc lập qua **FreeRTOS**:
- **Core 0 (Não bộ - AI Task):** Đảm nhiệm việc đọc tín hiệu Sensor (Mic, Touch, IMU, Nhiệt), lượng tử hóa dữ liệu, chạy phương trình Bellman (Q-Learning) để đưa ra quyết định cảm xúc.
- **Core 1 (Khuôn mặt - Graphic Task):** Đảm nhiệm việc nội suy tọa độ (Lerp 3D Parallax) và dựng hình Vector bằng LovyanGFX. Tuyệt đối không chứa logic đọc/ghi Blocking.

Giao tiếp giữa 2 Core được thực hiện qua biến chia sẻ nguyên tử (Atomic Shared Variable) `volatile int targetEmotionCode`.

---

## 2. NHỮNG THÀNH TỰU KỸ THUẬT CỐT LÕI (CORE IMPLEMENTATIONS)

### 2.1. Đồ họa Đa lớp Đồng tâm (Concentric Layering)
* **Vấn đề:** Không thể dùng hàm vẽ Sprite mờ lề (Anti-aliasing) hoặc Easing Alpha của LovyanGFX vì ngốn RAM và giảm FPS trầm trọng.
* **Giải pháp:** Sử dụng thuật toán **Concentric Layering (Các lớp đồng tâm)** thông qua hàm tự viết `drawGradientAsymmetricRect()`. 
* **Cách hoạt động:** Dùng Scanline Rasterization kết hợp công thức phương trình Elip `x = r - r * sqrt(1 - (dy/r)^2)` để vẽ các đường `drawFastHLine`. Hình ảnh có cảm giác 3D nổi khối và viền siêu mượt do sự hòa trộn màu gradient theo chiều dọc (VGradient).

### 2.2. Trí tuệ Nhân tạo - Tabular Q-Learning Nhúng
* **Vấn đề:** Mạng Nơ-ron (Deep Learning) không thể tự huấn luyện (Training) trực tiếp trên ESP32 vì cạn kiệt RAM và không có bộ xử lý Tensor.
* **Giải pháp:** Sử dụng Học Tăng Cường (Reinforcement Learning) dạng bảng (Tabular Q-Table).
* **Cách hoạt động:** 
  - **State Space (Không gian trạng thái):** Các cảm biến Môi trường được lượng tử hóa (Quantization) thành các số nguyên rất nhỏ (Ví dụ: 0: Lạnh, 1: Bình thường, 2: Nóng). Tổng chỉ có 12 trạng thái tổ hợp.
  - **Action Space (Hành động):** 11 trạng thái biểu cảm (Happy, Sad, Angry, Sleep, Cry, Dizzy, Wink, v.v.).
  - **Q-Table:** Mảng `float qTable[12][8]` lưu trữ "Kinh nghiệm".
  - **Reward Function:** Đây là linh hồn tạo nên tính cách robot. Khuyến khích (Cộng điểm) khi robot có hành vi đúng (Được vuốt ve -> Cười), Phạt (Trừ điểm) nếu sai (Ồn ào -> Ngủ).
  - Thuật toán áp dụng *Epsilon-Greedy* (20% thời gian robot sẽ hành xử điên rồ để khám phá môi trường).
### 2.3. Hiệu ứng Hoạt họa Động (Dynamic VFX)
Thay vì sử dụng Sprite chuyển động truyền thống tốn kém bộ nhớ, các hiệu ứng sinh động được tính toán bằng Toán học ngay trong quá trình Render:
* **Khóc (Cry):** Tính toán chu kỳ `millis() % 1500` để vẽ hình chữ nhật bo đáy (Bán nguyệt dưới - shapeType 2) thu nhỏ dần tạo cảm giác giọt nước mắt rơi lệch nhịp.
* **Chóng mặt (Dizzy):** Cộng thêm đồ thị Hình Sin/Cos (`sin/cos`) vào `offsetX/offsetY` để khiến con mắt xoay mòng mòng liên tục.
* **Nháy mắt (Wink):** Ghi đè chỉ số `blinkFactor` riêng biệt cho từng mắt tại thời điểm vẽ (Mắt trái khép tịt `0.05f`, mắt phải giữ nguyên).

### 2.4. Sửa Lỗi Hoán Đổi Byte (Endianness Mismatch) của SPI DMA
* **Vấn đề:** Khi định nghĩa màu Xanh Lá (Green), màn hình lại hiển thị màu Xanh Dương (Blue) hoặc Tím (Magenta).
* **Nguyên nhân:** CPU ESP32 sử dụng Little-Endian, nhưng chuẩn SPI của màn hình LCD lại là Big-Endian. Khi thư viện DMA đẩy 16-bit màu (RGB565) qua dây cáp, Byte thấp và Byte cao bị đảo ngược. Điều này khiến dải màu Xanh Lá (nằm ở giữa 6-bit) bị cắt đôi và phân bổ lộn xộn vào kênh Đỏ (Red) và Xanh Dương (Blue).
* **Giải pháp RALL (Phải Tuân Thủ):** 
  - Tuyệt đối không dùng trực tiếp biến tĩnh với `tft.color565()`.
  - Khai báo mọi màu sắc gốc dưới dạng **HEX RGB888** (Ví dụ: `0x00DC00` cho Xanh Lá).
  - Đưa màu vào hàm `lerpColor()` tự viết. Bước cuối cùng của hàm này **BẮT BUỘC** phải có thao tác hoán đổi bit (Bitwise Swap):
    ```cpp
    uint16_t c = tft.color565(r, g, b);
    return (uint32_t)((c >> 8) | (c << 8)); // Khắc phục lỗi Endianness
    ```

---

## 3. QUY TẮC CỨNG DÀNH CHO BẤT KỲ AI ĐỌC/SỬA MÃ NGUỒN (GUARDRAILS)

Nếu bạn là tôi trong tương lai hoặc một AI Sparing Partner khác, bạn **BẮT BUỘC PHẢI TUÂN THỦ** các nguyên tắc sau:

1. **KHÔNG BLOCKING TRÊN CORE 1:** Mọi thao tác chờ đợi (delay, đọc I2C, kết nối WiFi) phải đưa vào Task của Core 0. Core 1 (`loop()`) chỉ được quyền chứa `updateFaceLogic()` và `renderToScreen()`.
2. **LƯỢNG TỬ HÓA CẢM BIẾN (QUANTIZATION):** Bất cứ khi nào thêm một cảm biến mới (Ví dụ IMU góc nghiêng), tuyệt đối KHÔNG lấy số đo thực tế (float/int lớn) đưa vào mạng AI. Phải viết hàm bóp nó thành trạng thái rời rạc (Ví dụ: `LEFT`, `CENTER`, `RIGHT`) để ngăn hiện tượng bùng nổ không gian trạng thái (State Space Explosion) gây tràn RAM.
3. **MẮT XÍCH MÀU SẮC:** Khi đổi màu giao diện, phải tuân thủ chuẩn RGB888 và thay đổi ở bảng `ĐỊNH NGHĨA DẢI MÀU`. Không được hardcode màu sắc rác trong các thân hàm vẽ.
4. **COMMENT THEO NGUYÊN TẮC "WHY":** Mọi hàm quan trọng phải có Block Comment giải thích tại sao lại viết như vậy, phục vụ nghiệp vụ gì. Không giải thích cú pháp C++.
5. **THÁI ĐỘ MÃ NGUỒN:** Cấm sử dụng dấu ba chấm `...` khi cung cấp đoạn mã để sửa đổi. Sửa phần nào thì cung cấp trọn vẹn Logic khối (Block) đó.

> *Tài liệu này được tạo ra để lưu giữ tư duy kiến trúc ban đầu. Hãy trân trọng và đừng phá vỡ nó.*
