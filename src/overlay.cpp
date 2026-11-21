#include "overlay.h"

#include <objidl.h>
#include <gdiplus.h>
#include <cmath>
#include <cstring>
#include <memory>

namespace {

constexpr const wchar_t* kOverlayClassName = L"OverlayLabelWindowClass";

using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);

BYTE ClampByte(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<BYTE>(v);
}

void AddRoundedRectPath(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float radius) {
    if (radius <= 0.5f) {
        path.AddRectangle(Gdiplus::RectF(x, y, w, h));
        return;
    }

    float maxR = (w < h ? w : h) * 0.5f;
    if (radius > maxR) {
        radius = maxR;
    }
    float d = radius * 2.0f;

    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void AddCyberHudPath(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float bevel) {
    float maxBevel = (w < h ? w : h) * 0.38f;
    if (bevel > maxBevel) bevel = maxBevel;
    if (bevel < 2.0f) bevel = 2.0f;

    Gdiplus::PointF pts[6] = {
        Gdiplus::PointF(x + bevel, y),
        Gdiplus::PointF(x + w, y),
        Gdiplus::PointF(x + w, y + h - bevel),
        Gdiplus::PointF(x + w - bevel, y + h),
        Gdiplus::PointF(x, y + h),
        Gdiplus::PointF(x, y + bevel)
    };
    path.AddPolygon(pts, 6);
}

} // namespace

OverlayManager::~OverlayManager() {
    Shutdown();
}

bool OverlayManager::Initialize(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    Gdiplus::GdiplusStartupInput startupInput;
    if (Gdiplus::GdiplusStartup(&gdiplusToken_, &startupInput, nullptr) != Gdiplus::Ok) {
        return false;
    }
    gdiplusStarted_ = true;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = kOverlayClassName;

    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            Shutdown();
            return false;
        }
    }
    classRegistered_ = true;
    return true;
}

void OverlayManager::Shutdown() {
    DestroyAllWindows();

    if (classRegistered_ && hInstance_) {
        UnregisterClassW(kOverlayClassName, hInstance_);
        classRegistered_ = false;
    }

    if (gdiplusStarted_) {
        Gdiplus::GdiplusShutdown(gdiplusToken_);
        gdiplusStarted_ = false;
        gdiplusToken_ = 0;
    }
}

void OverlayManager::DestroyAllWindows() {
    if (draggingHwnd_) {
        if (GetCapture() == draggingHwnd_) {
            ReleaseCapture();
        }
        draggingHwnd_ = nullptr;
    }
    for (const auto& inst : windows_) {
        if (inst.hwnd && IsWindow(inst.hwnd)) {
            DestroyWindow(inst.hwnd);
        }
    }
    windows_.clear();
}

const OverlayManager::OverlayInstance* OverlayManager::FindInstance(HWND hwnd) const {
    for (const auto& inst : windows_) {
        if (inst.hwnd == hwnd) {
            return &inst;
        }
    }
    return nullptr;
}

void OverlayManager::ApplyDragStyleToWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (dragEnabled_) {
        exStyle &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    } else {
        exStyle |= static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    }
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(
        hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void OverlayManager::SetDragEnabled(bool enabled, DragCallback onDragged) {
    dragEnabled_ = enabled;
    onDragged_ = std::move(onDragged);
    if (!dragEnabled_ && draggingHwnd_) {
        if (GetCapture() == draggingHwnd_) {
            ReleaseCapture();
        }
        draggingHwnd_ = nullptr;
    }
    for (const auto& inst : windows_) {
        ApplyDragStyleToWindow(inst.hwnd);
    }
}

LRESULT CALLBACK OverlayManager::OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    OverlayManager* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<OverlayManager*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<OverlayManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleOverlayMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayManager::HandleOverlayMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NCHITTEST:
            return dragEnabled_ ? HTCLIENT : HTTRANSPARENT;

        case WM_SETCURSOR:
            if (dragEnabled_) {
                SetCursor(LoadCursorW(nullptr, IDC_SIZEALL));
                return TRUE;
            }
            break;

        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;

        case WM_ERASEBKGND:
            return 1;

        case WM_LBUTTONDOWN:
            if (dragEnabled_) {
                draggingHwnd_ = hwnd;
                GetCursorPos(&dragStartCursor_);
                GetWindowRect(hwnd, &dragStartRect_);
                lastDragPos_ = lastConfig_.position;
                lastDragMarginX_ = lastConfig_.marginX;
                lastDragMarginY_ = lastConfig_.marginY;
                SetCapture(hwnd);
                return 0;
            }
            break;

        case WM_MOUSEMOVE:
            if (dragEnabled_ && draggingHwnd_ == hwnd) {
                const OverlayInstance* inst = FindInstance(hwnd);
                if (!inst) return 0;

                POINT cur = {};
                GetCursorPos(&cur);

                int wndW = dragStartRect_.right - dragStartRect_.left;
                int wndH = dragStartRect_.bottom - dragStartRect_.top;
                int newX = dragStartRect_.left + (cur.x - dragStartCursor_.x);
                int newY = dragStartRect_.top + (cur.y - dragStartCursor_.y);

                const RECT& bounds = lastConfig_.respectTaskbar ? inst->target.workArea : inst->target.monitorArea;
                if (newX < bounds.left) newX = bounds.left;
                if (newY < bounds.top) newY = bounds.top;
                if (newX + wndW > bounds.right) newX = bounds.right - wndW;
                if (newY + wndH > bounds.bottom) newY = bounds.bottom - wndH;

                SetWindowPos(
                    hwnd, HWND_TOPMOST, newX, newY, 0, 0,
                    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

                int midX = (bounds.left + bounds.right) / 2;
                int midY = (bounds.top + bounds.bottom) / 2;
                int cx = newX + wndW / 2;
                int cy = newY + wndH / 2;
                bool isRight = (cx >= midX);
                bool isBottom = (cy >= midY);

                OverlayPosition pos = OverlayPosition::TopRight;
                int rawMx = 0;
                int rawMy = 0;
                if (!isBottom && isRight) {
                    pos = OverlayPosition::TopRight;
                    rawMx = bounds.right - (newX + wndW);
                    rawMy = newY - bounds.top;
                } else if (!isBottom && !isRight) {
                    pos = OverlayPosition::TopLeft;
                    rawMx = newX - bounds.left;
                    rawMy = newY - bounds.top;
                } else if (isBottom && isRight) {
                    pos = OverlayPosition::BottomRight;
                    rawMx = bounds.right - (newX + wndW);
                    rawMy = bounds.bottom - (newY + wndH);
                } else {
                    pos = OverlayPosition::BottomLeft;
                    rawMx = newX - bounds.left;
                    rawMy = bounds.bottom - (newY + wndH);
                }

                float dpiScale = static_cast<float>(GetMonitorDpi(inst->target.hMonitor)) / 96.0f;
                int mx = static_cast<int>(std::round(static_cast<float>(rawMx) / dpiScale));
                int my = static_cast<int>(std::round(static_cast<float>(rawMy) / dpiScale));
                if (mx < 0) mx = 0;
                if (mx > 500) mx = 500;
                if (my < 0) my = 0;
                if (my > 500) my = 500;

                lastDragPos_ = pos;
                lastDragMarginX_ = mx;
                lastDragMarginY_ = my;
                lastConfig_.position = pos;
                lastConfig_.marginX = mx;
                lastConfig_.marginY = my;

                if (onDragged_) {
                    onDragged_(pos, mx, my, false);
                }
                return 0;
            }
            break;

        case WM_LBUTTONUP:
            if (draggingHwnd_ == hwnd) {
                draggingHwnd_ = nullptr;
                if (GetCapture() == hwnd) {
                    ReleaseCapture();
                }
                if (onDragged_) {
                    onDragged_(lastDragPos_, lastDragMarginX_, lastDragMarginY_, true);
                }
                return 0;
            }
            break;

        case WM_CAPTURECHANGED:
            if (draggingHwnd_ == hwnd) {
                draggingHwnd_ = nullptr;
                if (onDragged_) {
                    onDragged_(lastDragPos_, lastDragMarginX_, lastDragMarginY_, true);
                }
                return 0;
            }
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

BOOL CALLBACK OverlayManager::EnumMonitorsProc(HMONITOR hMonitor, HDC /*hdc*/, LPRECT /*lprcMonitor*/, LPARAM dwData) {
    auto* targets = reinterpret_cast<std::vector<MonitorTarget>*>(dwData);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMonitor, &mi)) {
        MonitorTarget t;
        t.hMonitor = hMonitor;
        t.workArea = mi.rcWork;
        t.monitorArea = mi.rcMonitor;
        t.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        targets->push_back(t);
    }
    return TRUE;
}

UINT OverlayManager::GetMonitorDpi(HMONITOR hMonitor) {
    static HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    static GetDpiForMonitorFn pGetDpiForMonitor =
        hShcore ? reinterpret_cast<GetDpiForMonitorFn>(
                      reinterpret_cast<void*>(GetProcAddress(hShcore, "GetDpiForMonitor")))
                : nullptr;

    if (pGetDpiForMonitor) {
        UINT dpiX = 96;
        UINT dpiY = 96;
        if (SUCCEEDED(pGetDpiForMonitor(hMonitor, 0, &dpiX, &dpiY)) && dpiX >= 72) {
            return dpiX;
        }
    }

    HDC hdc = GetDC(nullptr);
    UINT dpi = hdc ? static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX)) : 96;
    if (hdc) {
        ReleaseDC(nullptr, hdc);
    }
    return dpi >= 72 ? dpi : 96;
}

HWND OverlayManager::CreateOverlayWindow() {
    DWORD exStyle =
        WS_EX_LAYERED |
        WS_EX_TOPMOST |
        WS_EX_TOOLWINDOW |
        WS_EX_NOACTIVATE;
    if (!dragEnabled_) {
        exStyle |= WS_EX_TRANSPARENT;
    }

    return CreateWindowExW(
        exStyle,
        kOverlayClassName,
        L"",
        WS_POPUP,
        0, 0, 1, 1,
        nullptr,
        nullptr,
        hInstance_,
        this);
}

void OverlayManager::RenderToWindow(HWND hwnd,
                                    const MonitorTarget& target,
                                    const OverlayConfig& config,
                                    const std::wstring& mainText,
                                    const std::wstring& subText) {
    UINT dpi = GetMonitorDpi(target.hMonitor);
    float dpiScale = static_cast<float>(dpi) / 96.0f;
    float userScale = static_cast<float>(config.scalePercent) / 100.0f;
    if (userScale < 0.5f) userScale = 0.5f;
    if (userScale > 2.5f) userScale = 2.5f;
    float scale = dpiScale * userScale;

    float mainFontPx = static_cast<float>(config.fontSize) * (static_cast<float>(dpi) / 72.0f) * userScale;
    float subPt = static_cast<float>(config.fontSize) * 0.56f;
    if (subPt < 8.5f) subPt = 8.5f;
    if (subPt > 16.0f) subPt = 16.0f;
    float subFontPx = subPt * (static_cast<float>(dpi) / 72.0f) * userScale;

    INT mainStyle = config.bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;

    Gdiplus::FontFamily requestedFamily(config.fontName.c_str());
    const Gdiplus::FontFamily* activeFamily = &requestedFamily;
    Gdiplus::FontFamily fallbackFamily(L"Segoe UI");
    if (requestedFamily.GetLastStatus() != Gdiplus::Ok) {
        activeFamily = &fallbackFamily;
    }

    Gdiplus::Font mainFont(activeFamily, mainFontPx, mainStyle, Gdiplus::UnitPixel);
    Gdiplus::Font subFont(activeFamily, subFontPx, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    if (mainFont.GetLastStatus() != Gdiplus::Ok) {
        return;
    }

    Gdiplus::StringFormat stringFormat(Gdiplus::StringFormat::GenericTypographic());
    stringFormat.SetFormatFlags(stringFormat.GetFormatFlags() |
                                Gdiplus::StringFormatFlagsMeasureTrailingSpaces |
                                Gdiplus::StringFormatFlagsNoWrap);

    Gdiplus::Bitmap measureBmp(1, 1, PixelFormat32bppPARGB);
    Gdiplus::Graphics measureG(&measureBmp);
    measureG.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    Gdiplus::RectF mainBounds;
    measureG.MeasureString(mainText.c_str(), -1, &mainFont, Gdiplus::PointF(0.0f, 0.0f), &stringFormat, &mainBounds);

    bool hasSub = !subText.empty();
    Gdiplus::RectF subBounds;
    if (hasSub) {
        measureG.MeasureString(subText.c_str(), -1, &subFont, Gdiplus::PointF(0.0f, 0.0f), &stringFormat, &subBounds);
    }

    bool isCyber = (config.frameStyle == FrameStyle::Cyber) && config.background;
    bool isPill = (config.frameStyle == FrameStyle::Pill) && config.background;
    bool hasBox = isCyber || isPill;

    float glowPad = isCyber ? (5.0f * scale) : (2.0f * scale);
    float padX = hasBox ? (static_cast<float>(config.paddingX) * scale) : (4.0f * scale);
    float padY = hasBox ? (static_cast<float>(config.paddingY) * scale) : (2.0f * scale);
    float shadowOffset = config.shadow ? (1.5f * scale) : 0.0f;

    float leftBarArea = isCyber ? (10.0f * scale) : 0.0f;
    float lineGap = hasSub ? (3.0f * scale) : 0.0f;

    float textW = (hasSub && subBounds.Width > mainBounds.Width ? subBounds.Width : mainBounds.Width) + shadowOffset;
    float textH = (hasSub ? (subBounds.Height + lineGap) : 0.0f) + mainBounds.Height + shadowOffset;

    std::unique_ptr<Gdiplus::Image> logoImg;
    float logoW = 0.0f;
    float logoH = 0.0f;
    float logoGap = 0.0f;
    if (!config.logoPath.empty()) {
        auto img = std::make_unique<Gdiplus::Image>(config.logoPath.c_str());
        if (img->GetLastStatus() == Gdiplus::Ok && img->GetWidth() > 0 && img->GetHeight() > 0) {
            float targetH = textH > (26.0f * scale) ? textH : (26.0f * scale);
            float aspect = static_cast<float>(img->GetWidth()) / static_cast<float>(img->GetHeight());
            float targetW = targetH * aspect;
            float maxW = targetH * 2.2f;
            if (targetW > maxW) {
                targetW = maxW;
                targetH = targetW / aspect;
            }
            logoW = targetW;
            logoH = targetH;
            logoGap = 10.0f * scale;
            logoImg = std::move(img);
        }
    }

    float contentW = (logoImg ? (logoW + logoGap) : 0.0f) + textW;
    float contentH = textH > logoH ? textH : logoH;

    float panelW = leftBarArea + padX * 2.0f + contentW;
    float panelH = padY * 2.0f + contentH;
    if (isCyber && panelW < 136.0f * scale) {
        panelW = 136.0f * scale;
    }

    int width = static_cast<int>(std::ceil(panelW + glowPad * 2.0f));
    int height = static_cast<int>(std::ceil(panelH + glowPad * 2.0f));
    if (width < 12) width = 12;
    if (height < 12) height = 12;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBitmap || !bits) {
        if (hBitmap) DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }

    std::memset(bits, 0, static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    HGDIOBJ oldBitmap = SelectObject(memDC, hBitmap);

    {
        Gdiplus::Bitmap surface(width, height, width * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(bits));
        Gdiplus::Graphics g(&surface);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

        BYTE accR = GetRValue(config.accentColor);
        BYTE accG = GetGValue(config.accentColor);
        BYTE accB = GetBValue(config.accentColor);
        BYTE bgR = GetRValue(config.backgroundColor);
        BYTE bgG = GetGValue(config.backgroundColor);
        BYTE bgB = GetBValue(config.backgroundColor);

        float px = glowPad;
        float py = glowPad;

        if (isCyber && config.backgroundOpacity > 0) {
            float bevel = 11.0f * scale;
            Gdiplus::GraphicsPath hudPath;
            AddCyberHudPath(hudPath, px, py, panelW, panelH, bevel);

            // Soft neon outer glow behind panel
            {
                Gdiplus::Pen glow1(Gdiplus::Color(36, accR, accG, accB), 4.6f * scale);
                glow1.SetLineJoin(Gdiplus::LineJoinRound);
                g.DrawPath(&glow1, &hudPath);

                Gdiplus::Pen glow2(Gdiplus::Color(76, accR, accG, accB), 2.4f * scale);
                glow2.SetLineJoin(Gdiplus::LineJoinRound);
                g.DrawPath(&glow2, &hudPath);
            }

            // Dark carbon glass vertical gradient fill
            Gdiplus::Color topCol(
                config.backgroundOpacity,
                ClampByte(static_cast<int>(bgR) + 14),
                ClampByte(static_cast<int>(bgG) + 18),
                ClampByte(static_cast<int>(bgB) + 26));
            Gdiplus::Color botCol(config.backgroundOpacity, bgR, bgG, bgB);
            Gdiplus::LinearGradientBrush bgBrush(
                Gdiplus::PointF(px, py),
                Gdiplus::PointF(px, py + panelH),
                topCol,
                botCol);
            g.FillPath(&bgBrush, &hudPath);

            // Subtle backlit neon wash on left edge & top glass sheen
            g.SetClip(&hudPath);
            {
                Gdiplus::LinearGradientBrush sideGlow(
                    Gdiplus::PointF(px, py),
                    Gdiplus::PointF(px + panelW * 0.42f, py),
                    Gdiplus::Color(static_cast<BYTE>((static_cast<int>(config.backgroundOpacity) * 28) / 255), accR, accG, accB),
                    Gdiplus::Color(0, accR, accG, accB));
                g.FillRectangle(&sideGlow, px, py, panelW * 0.42f, panelH);

                Gdiplus::LinearGradientBrush sheen(
                    Gdiplus::PointF(px, py),
                    Gdiplus::PointF(px, py + panelH * 0.48f),
                    Gdiplus::Color(22, 255, 255, 255),
                    Gdiplus::Color(0, 255, 255, 255));
                g.FillRectangle(&sheen, px, py, panelW, panelH * 0.48f);
            }
            g.ResetClip();

            // Crisp neon border
            Gdiplus::Pen borderPen(Gdiplus::Color(225, accR, accG, accB), 1.35f * scale);
            borderPen.SetLineJoin(Gdiplus::LineJoinMiter);
            g.DrawPath(&borderPen, &hudPath);

            // Left glowing vertical accent bar
            float barX = px + 8.0f * scale;
            float barY = py + bevel * 0.78f;
            float barW = 3.2f * scale;
            float barH = panelH - bevel * 0.78f - 7.0f * scale;
            if (barH > 6.0f * scale) {
                Gdiplus::GraphicsPath barPath;
                AddRoundedRectPath(barPath, barX, barY, barW, barH, barW * 0.5f);
                Gdiplus::SolidBrush barBrush(Gdiplus::Color(245, accR, accG, accB));
                g.FillPath(&barBrush, &barPath);
            }

            // Sharp top-right HUD corner highlight bracket
            float brk = 9.0f * scale;
            Gdiplus::Pen brkPen(
                Gdiplus::Color(240,
                               ClampByte((static_cast<int>(accR) + 255) / 2),
                               ClampByte((static_cast<int>(accG) + 255) / 2),
                               ClampByte((static_cast<int>(accB) + 255) / 2)),
                1.8f * scale);
            g.DrawLine(&brkPen, px + panelW - brk, py + 0.6f * scale, px + panelW - 0.6f * scale, py + 0.6f * scale);
            g.DrawLine(&brkPen, px + panelW - 0.6f * scale, py + 0.6f * scale, px + panelW - 0.6f * scale, py + brk);
        } else if (isPill && config.backgroundOpacity > 0) {
            Gdiplus::GraphicsPath bgPath;
            float radius = static_cast<float>(config.cornerRadius) * scale;
            AddRoundedRectPath(bgPath, px, py, panelW, panelH, radius);

            Gdiplus::SolidBrush bgBrush(Gdiplus::Color(config.backgroundOpacity, bgR, bgG, bgB));
            g.FillPath(&bgBrush, &bgPath);

            Gdiplus::Pen subtleBorder(Gdiplus::Color(95, accR, accG, accB), 1.0f * scale);
            g.DrawPath(&subtleBorder, &bgPath);
        }

        // Content origin
        float contentX = px + leftBarArea + padX;
        float contentY = py + padY;

        if (logoImg) {
            float logoY = contentY + (contentH - logoH) * 0.5f;
            Gdiplus::ImageAttributes ia;
            ia.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
            g.DrawImage(
                logoImg.get(),
                Gdiplus::RectF(contentX, logoY, logoW, logoH),
                0.0f, 0.0f,
                static_cast<float>(logoImg->GetWidth()),
                static_cast<float>(logoImg->GetHeight()),
                Gdiplus::UnitPixel,
                &ia);
            g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
            contentX += logoW + logoGap;
        }

        float textStartY = contentY + (contentH - textH) * 0.5f;
        float mainY = textStartY;
        float mainX = contentX;

        if (hasSub) {
            // Accent-tinted crisp sub-label
            BYTE subR = ClampByte((static_cast<int>(accR) * 55 + 255 * 45) / 100);
            BYTE subG = ClampByte((static_cast<int>(accG) * 55 + 255 * 45) / 100);
            BYTE subB = ClampByte((static_cast<int>(accB) * 55 + 255 * 45) / 100);
            constexpr BYTE subAlpha = 220;

            if (config.shadow) {
                Gdiplus::SolidBrush subShadow(Gdiplus::Color(
                    150,
                    GetRValue(config.shadowColor),
                    GetGValue(config.shadowColor),
                    GetBValue(config.shadowColor)));
                g.DrawString(
                    subText.c_str(), -1, &subFont,
                    Gdiplus::PointF(contentX + 1.0f * scale, textStartY + 1.0f * scale),
                    &stringFormat, &subShadow);
            }

            Gdiplus::SolidBrush subBrush(Gdiplus::Color(subAlpha, subR, subG, subB));
            g.DrawString(subText.c_str(), -1, &subFont, Gdiplus::PointF(contentX, textStartY), &stringFormat, &subBrush);

            mainY = textStartY + subBounds.Height + lineGap;
        }

        if (config.shadow) {
            Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(
                195,
                GetRValue(config.shadowColor),
                GetGValue(config.shadowColor),
                GetBValue(config.shadowColor)));
            g.DrawString(
                mainText.c_str(), -1, &mainFont,
                Gdiplus::PointF(mainX + shadowOffset, mainY + shadowOffset),
                &stringFormat, &shadowBrush);
        }

        Gdiplus::SolidBrush textBrush(Gdiplus::Color(
            255,
            GetRValue(config.textColor),
            GetGValue(config.textColor),
            GetBValue(config.textColor)));
        g.DrawString(mainText.c_str(), -1, &mainFont, Gdiplus::PointF(mainX, mainY), &stringFormat, &textBrush);
    }

    const RECT& bounds = config.respectTaskbar ? target.workArea : target.monitorArea;
    int marginX = static_cast<int>(std::round(static_cast<float>(config.marginX) * dpiScale));
    int marginY = static_cast<int>(std::round(static_cast<float>(config.marginY) * dpiScale));

    int posX = bounds.right - width - marginX;
    int posY = bounds.top + marginY;

    switch (config.position) {
        case OverlayPosition::TopRight:
            posX = bounds.right - width - marginX;
            posY = bounds.top + marginY;
            break;
        case OverlayPosition::TopLeft:
            posX = bounds.left + marginX;
            posY = bounds.top + marginY;
            break;
        case OverlayPosition::BottomRight:
            posX = bounds.right - width - marginX;
            posY = bounds.bottom - height - marginY;
            break;
        case OverlayPosition::BottomLeft:
            posX = bounds.left + marginX;
            posY = bounds.bottom - height - marginY;
            break;
    }

    POINT ptDst = { posX, posY };
    SIZE wndSize = { width, height };
    POINT ptSrc = { 0, 0 };
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = config.opacity;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(hwnd, screenDC, &ptDst, &wndSize, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

void OverlayManager::Rebuild(const OverlayConfig& config) {
    lastConfig_ = config;
    DestroyAllWindows();

    std::vector<MonitorTarget> monitors;
    EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsProc, reinterpret_cast<LPARAM>(&monitors));

    std::wstring mainText = FormatOverlayText(config.format, config.uppercase);
    std::wstring subText = FormatOverlayText(config.subText, config.uppercase);

    for (const auto& m : monitors) {
        if (!config.allMonitors && !m.isPrimary) {
            continue;
        }
        HWND hwnd = CreateOverlayWindow();
        if (!hwnd) {
            continue;
        }
        RenderToWindow(hwnd, m, config, mainText, subText);
        ShowWindow(hwnd, visible_ ? SW_SHOWNOACTIVATE : SW_HIDE);
        if (visible_) {
            SetWindowPos(
                hwnd,
                HWND_TOPMOST,
                0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
        windows_.push_back(OverlayInstance{ hwnd, m });
    }
}

void OverlayManager::UpdateContent(const OverlayConfig& config) {
    lastConfig_ = config;
    if (windows_.empty()) {
        Rebuild(config);
        return;
    }

    std::wstring mainText = FormatOverlayText(config.format, config.uppercase);
    std::wstring subText = FormatOverlayText(config.subText, config.uppercase);

    for (const auto& inst : windows_) {
        if (!inst.hwnd || !IsWindow(inst.hwnd)) continue;
        if (inst.hwnd == draggingHwnd_) continue;
        RenderToWindow(inst.hwnd, inst.target, config, mainText, subText);
    }
}

void OverlayManager::SetVisible(bool visible) {
    visible_ = visible;
    for (const auto& inst : windows_) {
        if (!inst.hwnd || !IsWindow(inst.hwnd)) continue;
        ShowWindow(inst.hwnd, visible_ ? SW_SHOWNOACTIVATE : SW_HIDE);
        if (visible_) {
            SetWindowPos(
                inst.hwnd,
                HWND_TOPMOST,
                0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    }
}

void OverlayManager::EnsureTopMost() {
    if (!visible_) return;
    for (const auto& inst : windows_) {
        if (!inst.hwnd || !IsWindow(inst.hwnd)) continue;
        SetWindowPos(
            inst.hwnd,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }
}

HICON OverlayManager::CreateAppTrayIcon() const {
    if (!gdiplusStarted_) {
        return LoadIconW(nullptr, IDI_INFORMATION);
    }

    const int size = 32;
    Gdiplus::Bitmap bmp(size, size, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&bmp);
    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    g.Clear(Gdiplus::Color(0, 0, 0, 0));

    Gdiplus::GraphicsPath bgPath;
    AddRoundedRectPath(bgPath, 2.0f, 2.0f, 28.0f, 28.0f, 7.0f);
    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 22, 54, 74));
    g.FillPath(&bgBrush, &bgPath);

    Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
    g.FillEllipse(&whiteBrush, 11.0f, 6.5f, 10.0f, 10.0f);
    g.FillPie(&whiteBrush, 6.5f, 17.5f, 19.0f, 16.0f, 180.0f, 180.0f);

    HICON hIcon = nullptr;
    if (bmp.GetHICON(&hIcon) == Gdiplus::Ok && hIcon) {
        return hIcon;
    }
    return LoadIconW(nullptr, IDI_INFORMATION);
}
