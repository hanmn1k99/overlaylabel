<div align="center">

<img src="resources/logo.png" alt="Overlay Label Logo" width="96" height="96" />

# Overlay Label

**Phần mềm hiển thị nhãn tên người dùng phong cách Quán nét / Cyber HUD — Always On Top, Xuyên chuột hoàn toàn & In nhãn phòng máy A4 cho Windows**

[![Version](https://img.shields.io/badge/Version-v1.1.0-00B4D8?style=for-the-badge)](https://github.com/hanmn1k99/overlaylabel/releases)
[![Platform](https://img.shields.io/badge/Platform-Windows%207%20%7C%208%20%7C%2010%20%7C%2011-0078D6?style=for-the-badge&logo=windows&logoColor=white)](#cài-đặt--sử-dụng)
[![Language](https://img.shields.io/badge/C%2B%2B17-Win32%20API%20%2B%20GDI%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](#kiến-trúc-kỹ-thuật)
[![Size](https://img.shields.io/badge/Binary-~430%20KB%20%C2%B7%20Zero%20Runtime-22C55E?style=for-the-badge)](#tính-năng-nổi-bật)
[![Publisher](https://img.shields.io/badge/By-minhhan.net-1E4E68?style=for-the-badge)](https://minhhan.net)

</div>

---

## Tổng quan

**Overlay Label** là công cụ Windows viết bằng **C++17 / Win32 API thuần + GDI+**, không phụ thuộc `.NET`, `Python`, hay `Electron`. Phần mềm hiển thị tên tài khoản Windows (`username`) — hoặc nhãn tùy biến kèm logo, tên máy, đồng hồ thực — ở góc màn hình theo phong cách **Quán nét / Cyber Gaming HUD**, luôn nổi trên cùng (**Always On Top**) nhưng **xuyên chuột 100% (Click-Through)**, đi kèm công cụ **dàn trang A4 xuất PDF & in nhãn phòng máy hàng loạt** đồng bộ cùng phong cách.

---

## Tính năng nổi bật

### 1. Xuyên chuột tuyệt đối & Luôn nổi trên cùng
- **Click-Through hoàn toàn**: Sử dụng `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` kết hợp `WM_NCHITTEST → HTTRANSPARENT` và `WM_MOUSEACTIVATE → MA_NOACTIVATE`. Mọi thao tác click chuột, cuộn trang, kéo thả và phím bấm đều đi thẳng xuống ứng dụng bên dưới.
- **Không cướp focus, không chiếm chỗ**: Ẩn hoàn toàn khỏi thanh **Taskbar** và danh sách **`Alt + Tab`**.
- **Hiển thị siêu sắc nét**: Render bằng GDI+ per-pixel alpha (`PixelFormat32bppPARGB` + `UpdateLayeredWindow`) với khử răng cưa mượt trên mọi nền sáng/tối.
- **Giữ lớp phủ ổn định**: Tự động duy trì `HWND_TOPMOST` không kích hoạt cửa sổ (`SWP_NOACTIVATE`) và đồng hồ `{time}` cập nhật theo thời gian thực mỗi giây.

### 2. 6 mẫu giao diện (Template) dựng sẵn — đổi nhanh 1 click
| Mẫu giao diện (Template) | Kiểu khung | Tông màu chủ đạo | Nội dung mặc định |
| :--- | :--- | :--- | :--- |
| **Cyber Neon (Xanh)** *(mặc định)* | Khung vát góc Cyber + viền phát sáng neon | Đen carbon `#0B1220` · Viền `#00E5FF` | `ONLINE • {time}` + `{username}` (in hoa) |
| **VIP Gold (Hoàng kim)** | Khung vát góc Cyber + viền vàng kim | Đen trầm `#14110B` · Viền `#FFB800` | `VIP ZONE • {time}` + `{username}` (in hoa) |
| **Esports Red (Đỏ ROG)** | Khung vát góc Cyber + viền đỏ neon | Đen obsidian `#160B10` · Viền `#FF2E55` | `GAMING • {time}` + `{username}` (in hoa) |
| **Matrix Net (Xanh lá)** | Khung vát góc Cyber + font `Consolas` | Xanh tối `#081610` · Viền `#22D366` | `NET STATION • {time}` + `{username}` (in hoa) |
| **Kính mờ (Bo tròn)** | Khung bo tròn hiện đại (Pill) | Kính đen mờ `#121826` · Viền `#38BDF8` | `{username}` (1 dòng gọn gàng) |
| **Tối giản (Chỉ chữ)** | Không khung nền, chỉ chữ nổi đổ bóng | Chữ trắng đổ bóng sắc nét | `{username}` |

### 3. Kéo thả trực tiếp & Tự động lưu theo thời gian thực
- **Tự động lưu tức thì**: Mọi thay đổi trong cửa sổ **Cài đặt** (chọn Template, kéo thanh trượt, đổi màu, chọn font/logo, gõ nội dung, bật/tắt tự khởi động) đều áp dụng **live** và **tự động lưu ngay**, không cần bấm Lưu.
- **Kéo thả trực tiếp khung Overlay**: Khi cửa sổ **Cài đặt** đang mở, bạn có thể nắm chuột kéo thẳng khung Overlay trên màn hình đến vị trí mong muốn — tự tính góc gần nhất + lề `X/Y` và tự lưu ngay khi thả chuột. Đóng cửa sổ Cài đặt sẽ tự bật lại chế độ xuyên chuột 100%.

### 4. Tùy chỉnh Logo ảnh & Giao diện toàn diện
- **Chèn Logo tùy chọn**: Chọn ảnh bất kỳ (`.png`, `.jpg`, `.jpeg`, `.bmp`, `.gif`, `.ico`) hiển thị sắc nét (trong suốt không khung thừa, khử viền bicubic) trên **cả Overlay màn hình lẫn nhãn in PDF**.
- **Nội dung linh hoạt**: Tùy chỉnh **Dòng chính** và **Dòng phụ** (để trống dòng phụ để chuyển 1 dòng).
- **Vị trí & Tỉ lệ**: 4 góc màn hình, lề `X/Y`, **Kích thước tổng thể (`50% – 250%`)**, **Độ trong suốt (`0% – 90%`)**, chọn Font hệ thống & bảng màu cho **Màu chữ**, **Viền Neon**, **Màu nền**, **Màu bóng**.

### 5. In nhãn phòng máy hàng loạt (Dàn trang A4 & Xuất PDF trực tiếp)
Mở ngay bằng nút **`In nhãn (PDF)...`** trong cửa sổ **Cài đặt**:
- **Đánh số máy tự động**: Nhập **Tiền tố (Ô 1)** (ví dụ `MAY`, `VIP`, `PC`), **Dấu nối**, **Số (Ô 2) từ … đến …** (ví dụ `1 → 40`), hỗ trợ **Đệm số 0 (`01, 02...`)** và **Dòng phụ** (ví dụ `minhhan.net`).
- **Đồng bộ màu & Logo theo Overlay hiện tại**: Tự động dùng đúng Template, bảng màu, font chữ và logo bạn đang chọn.
- **4 cỡ dàn trang A4 chuẩn phòng máy để nhìn từ xa**:
  - **Cỡ Lớn — 2×4** (8 nhãn/trang A4 · ~9 × 6.5 cm · dễ đọc từ xa ~5m)
  - **Cỡ Đại — 1×3** (3 nhãn/trang A4 · ~18 × 8.5 cm · biển siêu to)
  - **Cỡ Vừa — 2×5** (10 nhãn/trang A4 · ~9 × 5.2 cm · dán màn hình)
  - **Cỡ Gọn — 3×6** (18 nhãn/trang A4 · ~6 × 4.2 cm · tiết kiệm giấy)
- **Xem trước trang A4 nhiều trang**: Cửa sổ **`Xem trước A4...`** phóng to/thu nhỏ thoải mái, lật từng trang trước khi in hoặc xuất file.
- **Xuất PDF 250 DPI độc lập & In trực tiếp**: Tạo file PDF chuẩn A4 trực tiếp bằng C++ (không cần máy in ảo hay thư viện ngoài), có tùy chọn **đường cắt đứt nét**, hoặc bấm **`In trực tiếp...`** tới bất kỳ máy in Windows nào.

### 6. Đa màn hình, DPI V2, System Tray gọn nhẹ & Portable
- **Đa màn hình & Per-Monitor DPI V2**: Hiển thị trên tất cả màn hình hoặc chỉ màn hình chính, hỗ trợ né Taskbar, tự cập nhật khi đổi độ phân giải/cắm rút màn hình.
- **System Tray tinh gọn**: Click đúp để mở **Cài đặt**, menu chuột phải chỉ gồm 3 mục gọn gàng (**Cài đặt...**, **Hiển thị Overlay**, **Thoát**), tự khôi phục khi Explorer khởi động lại (`TaskbarCreated`).
- **Lưu cấu hình chuẩn Windows**: Lưu tại `%APPDATA%\OverlayLabel\config.ini` (chạy ổn định trong `Program Files` không cần quyền Admin). Hỗ trợ chế độ Portable nếu tạo file trống `portable.txt` cạnh file `.exe`.

---

## Biến động hỗ trợ (Placeholders)

| Biến | Giá trị hiển thị | Ghi chú |
| :--- | :--- | :--- |
| `{username}` | Tên tài khoản Windows hiện tại | Lấy qua `GetUserNameW` |
| `{computer}` | Tên máy tính (Hostname) | Lấy qua `GetComputerNameW` |
| `{time}` | Giờ hiện tại (`HH:mm`) | Cập nhật theo thời gian thực mỗi giây |
| `{date}` | Ngày hiện tại (`dd/MM/yyyy`) | Cập nhật tự động theo giờ hệ thống |

---

## Cài đặt & Sử dụng

### Cách 1: Dùng bộ cài đặt Windows *(khuyên dùng)*
1. Tải **`OverlayLabel-Setup-1.1.0.exe`** tại trang [Releases](https://github.com/hanmn1k99/overlaylabel/releases).
2. Chạy bộ cài đặt, chọn cài cho **riêng bạn** (không cần quyền Admin) hoặc **tất cả người dùng** (`Program Files`).
3. Gỡ cài đặt gọn gàng qua **Windows Settings → Apps → Overlay Label** (tự động đóng tiến trình đang chạy và xóa khóa tự khởi động).

### Cách 2: Chạy trực tiếp (Portable)
- Tải **`OverlayLabel.exe`** tại trang [Releases](https://github.com/hanmn1k99/overlaylabel/releases) và chạy trực tiếp (~430 KB, không cần cài đặt).

---

## Kiến trúc kỹ thuật

| Thành phần | Công nghệ / Win32 API |
| :--- | :--- |
| **Ngôn ngữ** | C++17 (Static CRT / Static libgcc & libstdc++ — 1 file `.exe` độc lập) |
| **Cửa sổ Overlay** | `CreateWindowExW` (`WS_EX_LAYERED`, `WS_EX_TRANSPARENT`, `WS_EX_TOPMOST`, `WS_EX_TOOLWINDOW`, `WS_EX_NOACTIVATE`) |
| **Đồ họa & Alpha** | GDI+ (`PixelFormat32bppPARGB`) + `CreateDIBSection` + `UpdateLayeredWindow` (`ULW_ALPHA`) |
| **Xuất PDF & In ấn** | Native PDF 1.4 writer (`/DCTDecode` 250 DPI A4 qua GDI+ JPEG encoder + `IStream`) & `PrintDlgW` / `StartDocW` |
| **Đa màn hình & DPI** | `EnumDisplayMonitors`, `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)`, `GetDpiForMonitor` |
| **Giao diện Cài đặt** | Win32 Native Controls + Common Controls v6 (`ActCtx`), `ChooseFontW`, `ChooseColorW`, `GetOpenFileNameW` |
| **Đóng gói Installer** | Inno Setup 6 (`LZMA2/max`), tự đóng tiến trình cũ qua `WM_CLOSE` khi nâng cấp/gỡ cài |

---

## Hướng dẫn biên dịch (Build from Source)

### Biên dịch file chạy `OverlayLabel.exe`
```bat
build.bat
```
File xuất ra tại `build\OverlayLabel.exe` (tự động đổi tên bản cũ sang `OverlayLabel.old.exe` nếu ứng dụng đang chạy).

### Biên dịch + Đóng gói bộ cài `OverlayLabel-Setup-1.1.0.exe`
```bat
build_installer.bat
```
File cài đặt xuất ra tại `dist\OverlayLabel-Setup-1.1.0.exe`.

---

## Cấu trúc thư mục

```text
overlaylabel/
├── src/
│   ├── main.cpp              # Điểm vào wWinMain, single-instance, System Tray, đồng hồ 1s
│   ├── overlay.h / .cpp      # Overlay đa màn hình, kéo thả trực tiếp & vẽ Cyber HUD bằng GDI+
│   ├── settings_window.h/.cpp# Cửa sổ Cài đặt trực quan (auto-save, 6 Template, chọn Logo)
│   ├── label_printer.h/.cpp  # Dàn trang A4, xuất PDF 250 DPI thuần C++ & in trực tiếp
│   ├── print_dialog.h / .cpp # Hộp thoại In nhãn phòng máy & cửa sổ Xem trước A4 nhiều trang
│   ├── config.h / .cpp       # Đọc/ghi config.ini (%APPDATA% / portable), Template & biến động
│   └── autostart.h / .cpp    # Bật/tắt tự khởi động cùng Windows qua Registry
├── resources/
│   ├── app.ico               # Icon ứng dụng đa kích thước (16–256px)
│   ├── logo.png              # Logo hiển thị README (256x256)
│   ├── app.manifest          # Manifest Common Controls v6 & DPI Per-Monitor V2
│   └── app.rc                # Nhúng icon, manifest và Version Info 1.1.0.0
├── installer/
│   └── OverlayLabel.iss      # Kịch bản đóng gói Inno Setup 6
├── CMakeLists.txt
├── build.bat
├── build_installer.bat
└── README.md
```

---

<div align="center">
Phát triển bởi <a href="https://minhhan.net"><strong>minhhan.net</strong></a>
</div>
