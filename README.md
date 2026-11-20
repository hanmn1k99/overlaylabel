<div align="center">

<img src="resources/logo.png" alt="Overlay Label Logo" width="96" height="96" />

# Overlay Label

**Phần mềm hiển thị nhãn tên người dùng phong cách Quán nét / Cyber HUD — Always On Top & Xuyên chuột hoàn toàn cho Windows**

[![Version](https://img.shields.io/badge/Version-v1.0.0-00B4D8?style=for-the-badge)](https://github.com/hanmn1k99/overlaylabel/releases)
[![Platform](https://img.shields.io/badge/Platform-Windows%207%20%7C%208%20%7C%2010%20%7C%2011-0078D6?style=for-the-badge&logo=windows&logoColor=white)](#cài-đặt--sử-dụng)
[![Language](https://img.shields.io/badge/C%2B%2B17-Win32%20API%20%2B%20GDI%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](#kiến-trúc-kỹ-thuật)
[![Size](https://img.shields.io/badge/Binary-~400%20KB%20%C2%B7%20Zero%20Runtime-22C55E?style=for-the-badge)](#tính-năng-nổi-bật)
[![Publisher](https://img.shields.io/badge/By-minhhan.net-1E4E68?style=for-the-badge)](https://minhhan.net)

</div>

---

## Tổng quan

**Overlay Label** là công cụ Windows viết bằng **C++17 / Win32 API thuần + GDI+**, không phụ thuộc `.NET`, `Python`, hay `Electron`. Phần mềm hiển thị tên tài khoản Windows (`username`) — hoặc nhãn tùy biến kèm tên máy, đồng hồ thực — ở góc màn hình theo phong cách **Quán nét / Cyber Gaming HUD**, luôn nổi trên cùng (**Always On Top**) nhưng **xuyên chuột 100% (Click-Through)**, hoàn toàn không cản trở thao tác làm việc, học tập hay chơi game.

---

## Tính năng nổi bật

### 1. Xuyên chuột tuyệt đối & Luôn nổi trên cùng
- **Click-Through hoàn toàn**: Sử dụng `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` kết hợp `WM_NCHITTEST → HTTRANSPARENT` và `WM_MOUSEACTIVATE → MA_NOACTIVATE`. Mọi thao tác click chuột, cuộn trang, kéo thả và phím bấm đều đi thẳng xuống ứng dụng bên dưới.
- **Không cướp focus, không chiếm chỗ**: Ẩn hoàn toàn khỏi thanh **Taskbar** và danh sách **`Alt + Tab`**.
- **Hiển thị siêu sắc nét**: Render bằng GDI+ per-pixel alpha (`PixelFormat32bppPARGB` + `UpdateLayeredWindow`) với khử răng cưa mượt trên mọi nền sáng/tối và không bị răng cưa viền như kiểu color-key cũ.
- **Giữ lớp phủ ổn định**: Tự động duy trì trạng thái `HWND_TOPMOST` mà không kích hoạt cửa sổ (`SWP_NOACTIVATE`).

### 2. 6 mẫu giao diện (Template) dựng sẵn — đổi nhanh 1 click
Chọn trực tiếp trong cửa sổ **Cài đặt** (xem trước trực tiếp) hoặc ngay từ menu chuột phải **System Tray → Mẫu giao diện**:

| Mẫu giao diện (Template) | Kiểu khung | Tông màu chủ đạo | Nội dung mặc định |
| :--- | :--- | :--- | :--- |
| **Cyber Neon (Xanh)** *(mặc định)* | Khung vát góc Cyber + viền phát sáng neon | Đen carbon `#0B1220` · Viền `#00E5FF` | `● ONLINE • {time}` + `{username}` (in hoa) |
| **VIP Gold (Hoàng kim)** | Khung vát góc Cyber + viền vàng kim | Đen trầm `#14110B` · Viền `#FFB800` | `● VIP ZONE • {time}` + `{username}` (in hoa) |
| **Esports Red (Đỏ ROG)** | Khung vát góc Cyber + viền đỏ neon | Đen obsidian `#160B10` · Viền `#FF2E55` | `● GAMING • {time}` + `{username}` (in hoa) |
| **Matrix Net (Xanh lá)** | Khung vát góc Cyber + font `Consolas` | Xanh tối `#081610` · Viền `#22D366` | `● NET STATION • {time}` + `{username}` (in hoa) |
| **Kính mờ (Bo tròn)** | Khung bo tròn hiện đại (Pill) | Kính đen mờ `#121826` · Viền `#38BDF8` | `● {username}` (1 dòng gọn gàng) |
| **Tối giản (Chỉ chữ)** | Không khung nền, chỉ chữ nổi đổ bóng | Chữ trắng đổ bóng sắc nét | `{username}` |

### 3. Cửa sổ Cài đặt trực quan (Live Preview)
Không cần sửa tay file cấu hình — mọi thay đổi đều hiển thị **xem trước trực tiếp trên màn hình**:
- **Mẫu giao diện**: 6 nút chuyển nhanh Template ngay trên đầu cửa sổ.
- **Nội dung linh hoạt**: Tùy chỉnh **Dòng chính** và **Dòng phụ** (để trống dòng phụ để tự động chuyển sang chế độ 1 dòng).
- **Vị trí & Căn lề**: Chọn 4 góc màn hình (`Trên phải`, `Trên trái`, `Dưới phải`, `Dưới trái`), kéo thanh trượt **Lề ngang (X)** và **Lề dọc (Y)** (`0 – 200 px`).
- **Kiểu dáng & Kích thước**:
  - Chọn kiểu khung (**Cyber Quán nét**, **Bo tròn**, **Chỉ chữ**), bật/tắt **In hoa**, **Chấm Online (`●`)**, **In đậm**, **Đổ bóng**.
  - Thanh trượt **Kích thước tổng thể (`50% – 250%`)** phóng to/thu nhỏ đồng bộ toàn bộ khung HUD mà vẫn giữ nguyên khoảng cách bám góc.
  - Thanh trượt **Độ trong suốt (`0% – 90%`)** áp dụng đồng bộ lên toàn bộ overlay (khung, viền neon, chấm trạng thái và chữ).
  - Bảng chọn **Font hệ thống** (`ChooseFont`) và **Bảng màu trực quan** (`ChooseColor`) cho **Màu chữ**, **Viền Neon**, **Màu nền**, **Màu bóng**.
- **Nút thao tác**: **Mặc định** (khôi phục cài đặt gốc), **Lưu** (lưu vĩnh viễn), **Hủy** (hoàn nguyên mọi thay đổi chưa lưu). Hỗ trợ phím `Tab`, `Enter` (Lưu), `Esc` (Hủy).

### 4. Đa màn hình & Per-Monitor DPI V2
- Hỗ trợ hiển thị trên **tất cả màn hình** hoặc **chỉ màn hình chính**.
- Tùy chọn **Né Taskbar** (`rcWork`) hoặc bám sát mép màn hình (`rcMonitor`).
- Hỗ trợ **Per-Monitor DPI V2**: Chữ và khung luôn sắc nét trên các màn hình có tỷ lệ scale khác nhau (ví dụ màn hình 4K 150% đi kèm màn hình Full HD 100%).
- Tự động căn chỉnh lại khi cắm/rút màn hình, thay đổi độ phân giải (`WM_DISPLAYCHANGE`) hoặc thay đổi thanh Taskbar (`SPI_SETWORKAREA`).

### 5. System Tray thông minh & Tự khởi động cùng Windows
- **Click đúp** icon ở khay hệ thống (hoặc mở lại `OverlayLabel.exe` khi app đang chạy) để mở nhanh cửa sổ **Cài đặt**.
- **Chuột phải** icon để mở menu: **Cài đặt...**, **Mẫu giao diện**, **Hiển thị Overlay**, **Hiển thị trên tất cả màn hình**, **Khởi động cùng Windows**, và **Thoát**.
- **Tự khôi phục icon Tray**: Tự động đăng ký lại biểu tượng nếu `explorer.exe` khởi động lại (`TaskbarCreated`).
- **Chống chạy trùng (Single-Instance)**: Dùng `CreateMutexW` đảm bảo chỉ có 1 tiến trình chạy duy nhất.
- **Khởi động cùng Windows**: Lưu tại `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\OverlayLabel` — không cần quyền Administrator.

### 6. Lưu cấu hình chuẩn Windows & Hỗ trợ Portable
- **Mặc định (bản cài đặt)**: Tự động lưu cấu hình tại `%APPDATA%\OverlayLabel\config.ini`, hoạt động ổn định cho mọi tài khoản user ngay cả khi cài trong `C:\Program Files\Overlay Label\`.
- **Chế độ Portable**: Tạo một file trống tên `portable.txt` đặt cùng thư mục với `OverlayLabel.exe` để phần mềm lưu `config.ini` ngay cạnh file chạy.

---

## Biến động hỗ trợ (Placeholders)

Bạn có thể chèn các biến sau vào **Dòng chính** hoặc **Dòng phụ** trong cửa sổ Cài đặt:

| Biến | Giá trị hiển thị | Ghi chú |
| :--- | :--- | :--- |
| `{username}` | Tên tài khoản Windows hiện tại | Lấy qua `GetUserNameW` |
| `{computer}` | Tên máy tính (Hostname) | Lấy qua `GetComputerNameW` |
| `{time}` | Giờ hiện tại (`HH:mm`) | Tự động cập nhật theo phút, không nhấp nháy |
| `{date}` | Ngày hiện tại (`dd/MM/yyyy`) | Cập nhật tự động theo giờ hệ thống |

**Ví dụ:**
- Dòng chính: `{username}` · Dòng phụ: `ONLINE • {time}`
- Dòng chính: `{computer}` · Dòng phụ: `USER: {username} • {date}`
- Dòng chính: `MINH HÂN NET • {username}` · Dòng phụ: *(để trống)*

---

## Cài đặt & Sử dụng

### Cách 1: Dùng bộ cài đặt Windows *(khuyên dùng)*
1. Tải **`OverlayLabel-Setup-1.0.0.exe`** tại trang [Releases](https://github.com/hanmn1k99/overlaylabel/releases).
2. Chạy bộ cài đặt, chọn cài cho **riêng bạn** (không cần quyền Admin) hoặc **tất cả người dùng** (`Program Files`).
3. Tích chọn **Tạo biểu tượng Desktop** và **Khởi động cùng Windows** nếu muốn.
4. Gỡ cài đặt gọn gàng bất kỳ lúc nào qua **Windows Settings → Apps → Overlay Label** (tự động đóng ứng dụng đang chạy và xóa khóa tự khởi động).

### Cách 2: Chạy trực tiếp (Portable)
- Tải **`OverlayLabel.exe`** tại trang [Releases](https://github.com/hanmn1k99/overlaylabel/releases) và chạy trực tiếp (~400 KB, không cần cài đặt).

---

## Kiến trúc kỹ thuật

| Thành phần | Công nghệ / Win32 API |
| :--- | :--- |
| **Ngôn ngữ** | C++17 (Static CRT / Static libgcc & libstdc++ — 1 file `.exe` độc lập) |
| **Cửa sổ Overlay** | `CreateWindowExW` (`WS_EX_LAYERED`, `WS_EX_TRANSPARENT`, `WS_EX_TOPMOST`, `WS_EX_TOOLWINDOW`, `WS_EX_NOACTIVATE`) |
| **Đồ họa & Alpha** | GDI+ (`PixelFormat32bppPARGB`) + `CreateDIBSection` + `UpdateLayeredWindow` (`ULW_ALPHA`) |
| **Đa màn hình & DPI** | `EnumDisplayMonitors`, `GetMonitorInfoW`, `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)`, `GetDpiForMonitor` |
| **Giao diện Cài đặt** | Win32 Native Controls + Common Controls v6 (`ActCtx`), `ChooseFontW`, `ChooseColorW` |
| **Khay hệ thống & Khởi động** | `Shell_NotifyIconW`, `TaskbarCreated`, Registry `HKCU\...\Run` |
| **Đóng gói Installer** | Inno Setup 6 (`LZMA2/max`), tự đóng tiến trình cũ qua `WM_CLOSE` khi nâng cấp/gỡ cài |

> **Lưu ý về bảo mật & ứng dụng Fullscreen:** Overlay Label chỉ đọc tên user/tên máy từ Win32 API chuẩn để hiển thị nhãn tĩnh, không đọc nội dung màn hình, không hook phím/chuột, không can thiệp trình duyệt hay tiến trình khác.

---

## Hướng dẫn biên dịch (Build from Source)

### Yêu cầu
- **MinGW-w64 (`g++`)** *(hoặc Visual Studio / MSVC Build Tools)* — cài nhanh:
  ```powershell
  winget install BrechtSanders.WinLibs.POSIX.UCRT
  ```
- **Inno Setup 6** *(chỉ cần nếu muốn đóng gói lại file Setup)*:
  ```powershell
  winget install --id JRSoftware.InnoSetup -e
  ```

### Biên dịch file chạy `OverlayLabel.exe`
```bat
build.bat
```
File xuất ra tại `build\OverlayLabel.exe` (tự động đổi tên bản cũ sang `OverlayLabel.old.exe` nếu ứng dụng đang chạy nên không bị lỗi khóa file).

Hoặc dùng **CMake**:
```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Biên dịch + Đóng gói bộ cài `OverlayLabel-Setup-1.0.0.exe`
```bat
build_installer.bat
```
File cài đặt xuất ra tại `dist\OverlayLabel-Setup-1.0.0.exe`.

---

## Cấu trúc thư mục

```text
overlaylabel/
├── src/
│   ├── main.cpp              # Điểm vào wWinMain, single-instance, System Tray, timer đồng hồ
│   ├── overlay.h / .cpp      # Quản lý overlay đa màn hình & vẽ Cyber HUD bằng GDI+
│   ├── settings_window.h/.cpp# Cửa sổ Cài đặt trực quan có Live Preview & 6 Template
│   ├── config.h / .cpp       # Đọc/ghi config.ini, quản lý Template & biến động
│   └── autostart.h / .cpp    # Bật/tắt tự khởi động cùng Windows qua Registry
├── resources/
│   ├── app.ico               # Icon ứng dụng đa kích thước (16–256px)
│   ├── logo.png              # Logo hiển thị README (256x256)
│   ├── app.manifest          # Manifest Common Controls v6 & DPI Per-Monitor V2
│   └── app.rc                # Nhúng icon, manifest và Version Info 1.0.0.0
├── installer/
│   └── OverlayLabel.iss      # Kịch bản đóng gói Inno Setup 6
├── CMakeLists.txt            # Cấu hình build CMake (MSVC & MinGW-w64)
├── build.bat                 # Script build nhanh OverlayLabel.exe
├── build_installer.bat       # Script build + đóng gói Installer
└── README.md
```

---

<div align="center">
Phát triển bởi <a href="https://minhhan.net"><strong>minhhan.net</strong></a>
</div>
