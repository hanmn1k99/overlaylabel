# Overlay Label (C++ Win32 API)

Phần mềm Windows siêu nhẹ viết bằng **C++ / Win32 API thuần + GDI+**, hiển thị nhãn tên người dùng (username) theo phong cách **Quán nét / Cyber HUD** ở góc trên bên phải màn hình, **luôn nằm trên cùng (Always On Top)** và **xuyên chuột hoàn toàn (Click-Through)** — không cản bất kỳ thao tác chuột hay bàn phím nào.

## Tính năng chính

- **Phong cách Quán nét / Cyber HUD với 6 mẫu Template có sẵn**:
  - **Cyber Neon (Xanh)**: Khung vát góc đen kính mờ, viền neon xanh cyan, chấm online xanh lá, dòng phụ `ONLINE • {time}`.
  - **VIP Gold (Hoàng kim)**: Phong cách phòng VIP đen carbon viền vàng kim.
  - **Esports Red (Đỏ ROG)**: Khung vát góc gaming đỏ neon.
  - **Matrix Net (Xanh lá)**: Kiểu phòng máy cổ điển font Consolas xanh lá.
  - **Kính mờ (Bo tròn)**: Khung bo tròn hiện đại 1 dòng gọn gàng.
  - **Tối giản (Chỉ chữ)**: Chỉ hiển thị chữ nổi có đổ bóng, không nền.
- **Click-Through & Always On Top**: Sử dụng `WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` + `UpdateLayeredWindow` (per-pixel alpha khử răng cưa mượt), ẩn khỏi Taskbar và `Alt+Tab`, không cướp focus.
- **Cửa sổ Cài đặt trực quan (Live Preview)**:
  - Mở nhanh bằng cách **click đúp icon System Tray**, chuột phải → **Cài đặt...**, hoặc mở lại `OverlayLabel.exe` khi ứng dụng đang chạy.
  - Chọn nhanh 6 mẫu Template, chỉnh dòng chính/dòng phụ (hỗ trợ `{username}`, `{computer}`, `{time}`, `{date}`), vị trí 4 góc, lề X/Y, kiểu khung, font/cỡ chữ, màu chữ/viền neon/nền/bóng, độ mờ, đa màn hình, né Taskbar và tự khởi động cùng Windows — xem trước trực tiếp trên màn hình rồi bấm **Lưu**.
- **System Tray Icon**: Đổi nhanh 6 mẫu Template ngay từ menu chuột phải, bật/tắt hiển thị, đa màn hình, khởi động cùng Windows, thoát ứng dụng.
- **Đa màn hình & DPI Per-Monitor V2**: Hỗ trợ nhiều màn hình với mức scale khác nhau, tự cập nhật khi thay đổi độ phân giải/màn hình.
- **Không cần runtime**: Biên dịch ra 1 file `OverlayLabel.exe` độc lập.

## Cách biên dịch (Build)

Chạy trực tiếp trong thư mục dự án:

```bat
build.bat
```

File xuất ra tại `build\OverlayLabel.exe`.
