# Macro Recorder K (C++ Win32 Core & GUI Application)

Dự án **Macro Recorder** hiệu năng cao được xây dựng từ con số 0 bằng **C++17** và **Windows Win32 Low-Level API**. Dự án tập trung vào độ trễ cực thấp, độ chính xác cấp microsecond, khả năng kiểm soát phần cứng trực tiếp và giao diện đồ họa trực quan (GUI).

---

## 🚀 Tính năng nổi bật

1. **Giao diện đồ họa trực quan (Native Win32 GUI):**
   - Chạy độc lập, siêu nhẹ (~260 KB), mở tức thì (< 50ms), không cần cài đặt Python hay .NET runtime.
   - Bảng điều khiển thời gian thực: Danh sách sự kiện trực quan (Table ListView) cập nhật từng phím bấm và cú click chuột ngay khi đang ghi âm.
   - Hộp thoại Mở / Lưu file JSON chuẩn Windows (`GetOpenFileName` / `GetSaveFileName`).

2. **Cài đặt số vòng lặp & Lặp vô hạn (Infinite Loop):**
   - **Số vòng lặp = 0**: Chế độ **Lặp Vô Hạn** (Infinite Looping). Macro sẽ tự động lặp lại liên tục không ngừng cho đến khi người dùng bấm phím **`[ESC]`** (hoặc **`[F9]`**) để dừng.
   - **Số vòng lặp >= 1**: Lặp lại đúng số lần quy định.
   - **Nghỉ giữa các vòng (ms)**: Tùy chỉnh độ trễ trước khi bắt đầu vòng lặp tiếp theo.

3. **Ghi nhận sự kiện thời gian thực (Low-Level Hooks):**
   - Chuột: Di chuyển (`WM_MOUSEMOVE`), Click trái/phải/giữa/X-buttons (`WM_LBUTTONDOWN`, `WM_RBUTTONDOWN`,...), Cuộn bánh xe dọc & ngang (`WM_MOUSEWHEEL`, `WM_MOUSEHWHEEL`).
   - Bàn phím: Bắt mọi phím bấm (`WM_KEYDOWN`, `WM_KEYUP`, phím mở rộng `isExtendedKey`, Virtual-Key code và Scan code).
   - Lọc thông minh: Loại bỏ sự kiện giả lập (`LLKHF_INJECTED`, `LLMHF_INJECTED`), giảm bớt sự kiện di chuyển chuột thừa (throttle mouse moves).

4. **Phát lại chuẩn xác cấp mili-giây (High-Precision Playback):**
   - Sử dụng `QueryPerformanceCounter` và Windows Timer resolution 1ms (`timeBeginPeriod`).
   - Giả lập bằng `SendInput` chuẩn Win32 hỗ trợ đa màn hình (Multi-monitor Virtual Desktop coordinates).
   - Hỗ trợ tăng tốc độ (0.5x, 1x, 1.5x, 2x, 3x, 5x, 10x,...).

5. **Phím tắt toàn cục (Global Hotkeys):**
   - **`[F8]`**: Bắt đầu / Dừng ghi macro mọi lúc mọi nơi.
   - **`[F9]`**: Bắt đầu phát lại / Dừng phát lại macro.
   - **`[ESC]`**: Dừng khẩn cấp (Emergency Stop) ngay lập tức (đặc biệt hữu ích khi đang lặp vô hạn).
   - Các phím điều khiển này tự động bị bỏ qua, không bị ghi nhầm vào chuỗi macro.

---

## 📁 Cấu trúc thư mục

```
macro_K/
├── src/
│   ├── core/
│   │   ├── Types.h            # Định nghĩa cấu trúc sự kiện chuột, phím, cấu hình
│   │   ├── PrecisionTimer.h   # Bộ đếm thời gian độ chính xác cao (QueryPerformanceCounter)
│   │   ├── HookManager.h/.cpp # Quản lý Hook bàn phím/chuột và vòng lặp thông điệp
│   │   ├── MacroPlayer.h/.cpp # Bộ phát lại sự kiện sử dụng Win32 SendInput
│   │   ├── MacroStorage.h/.cpp# Đọc/ghi macro định dạng JSON độc lập
│   │   └── MacroCoreApi.h/.cpp# C-ABI DLL xuất các hàm điều khiển
│   ├── gui/
│   │   ├── main_gui.cpp       # Ứng dụng giao diện đồ họa Win32 GUI
│   │   ├── app.manifest       # Cấu hình Visual Styles & High-DPI scaling
│   │   └── resource.rc        # Resource nhúng manifest
│   ├── cli/
│   │   └── main.cpp           # Giao diện dòng lệnh (CLI)
│   └── tests/
│       └── test_core.cpp      # Bộ kiểm thử đơn vị tự động (Unit Test)
├── macros/
│   └── sample_macro.json      # File macro mẫu
├── bin/
│   ├── macro_k_gui.exe        # Ứng dụng Giao diện đồ họa (GUI)
│   ├── macro_k.exe            # Ứng dụng Giao diện dòng lệnh (CLI)
│   ├── macro_k_core.dll       # Thư viện Core DLL dùng chung
│   └── test_core.exe          # File chạy test
├── CMakeLists.txt             # File cấu hình build CMake
├── build.bat                  # Script biên dịch 1 click
└── README.md                  # Hướng dẫn chi tiết
```

---

## 🛠️ Hướng dẫn biên dịch

Chạy file script [`build.bat`](file:///c:/Users/Admin/Desktop/code/myself/macro_K/build.bat) trong thư mục gốc:
```cmd
build.bat
```
Script sẽ tự động biên dịch toàn bộ:
1. `bin\macro_k_gui.exe` (Bản giao diện đồ họa)
2. `bin\macro_k.exe` (Bản console CLI)
3. `bin\macro_k_core.dll` (Thư viện DLL)
4. `bin\test_core.exe` (Bộ test)

---

## 🎮 Hướng dẫn sử dụng Giao diện GUI

1. Nhấp đúp mở file `bin\macro_k_gui.exe`.
2. **Cấu hình Vòng lặp:**
   - Trong ô **Số vòng lặp**, nhập `0` nếu muốn **Lặp Vô Hạn** cho tới khi bấm **ESC**.
   - Hoặc nhập số nguyên dương (ví dụ `1`, `5`, `10`) để lặp đúng số lần.
3. **Ghi macro:**
   - Nhấn nút **● Bắt đầu Ghi (F8)** hoặc bấm phím tắt **F8**.
   - Thao tác chuột và bàn phím (sự kiện sẽ xuất hiện trực tiếp trên bảng danh sách).
   - Nhấn **■ Dừng Ghi (F8)** hoặc bấm **F8** để kết thúc.
4. **Phát lại:**
   - Nhấn nút **▶ Phát Macro (F9)** hoặc bấm **F9**.
   - Trong lúc phát (kể cả khi đang lặp vô hạn), bạn có thể nhấn **ESC** hoặc **F9** bất cứ lúc nào để dừng lại ngay lập tức.
5. **Lưu & Mở file:**
   - Bấm **💾 Lưu File...** để lưu vào thư mục `macros/`.
   - Bấm **📂 Mở File...** để nạp macro có sẵn.
