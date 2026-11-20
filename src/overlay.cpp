#include "overlay.h"

#include <objidl.h>
#include <gdiplus.h>
#include <cmath>
#include <cstring>

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
    for (const auto& inst : windows_) {
        if (inst.hwnd && IsWindow(inst.hwnd)) {
            DestroyWindow(inst.hwnd);
        }
    }
    windows_.clear();
}

LRESULT CALLBACK OverlayManager::OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NCHITTEST:
            return HTTRANSPARENT;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
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
    const DWORD exStyle =
        WS_EX_LAYERED |
        WS_EX_TRANSPARENT |
        WS_EX_TOPMOST |
        WS_EX_TOOLWINDOW |
        WS_EX_NOACTIVATE;

    return CreateWindowExW(
        exStyle,
        kOverlayClassName,
        L"",
        WS_POPUP,
        0, 0, 1, 1,
        nullptr,
        nullptr,
        hInstance_,
        nullptr);
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
    float dotSize = config.showStatusDot ? (7.5f * scale) : 0.0f;
    float dotGap = config.showStatusDot ? (7.0f * scale) : 0.0f;
    float lineGap = hasSub ? (3.0f * scale) : 0.0f;

    float row1W = hasSub ? ((config.showStatusDot ? (dotSize + dotGap) : 0.0f) + subBounds.Width) : 0.0f;
    float row2W = (!hasSub && config.showStatusDot ? (dotSize + dotGap) : 0.0f) + mainBounds.Width;
    float contentW = (row1W > row2W ? row1W : row2W) + shadowOffset;
    float contentH = (hasSub ? (subBounds.Height + lineGap) : 0.0f) + mainBounds.Height + shadowOffset;

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

        auto drawStatusDot = [&](float cx, float cy) {
            // Soft emerald green halo + bright core
            Gdiplus::SolidBrush halo(Gdiplus::Color(75, 34, 197, 94));
            g.FillEllipse(&halo, cx - dotSize * 0.78f, cy - dotSize * 0.78f, dotSize * 1.56f, dotSize * 1.56f);
            Gdiplus::SolidBrush core(Gdiplus::Color(255, 52, 232, 122));
            g.FillEllipse(&core, cx - dotSize * 0.5f, cy - dotSize * 0.5f, dotSize, dotSize);
        };

        float mainY = contentY;
        float mainX = contentX;

        if (hasSub) {
            float subX = contentX;
            if (config.showStatusDot) {
                drawStatusDot(subX + dotSize * 0.5f, contentY + subBounds.Height * 0.52f);
                subX += dotSize + dotGap;
            }

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
                    Gdiplus::PointF(subX + 1.0f * scale, contentY + 1.0f * scale),
                    &stringFormat, &subShadow);
            }

            Gdiplus::SolidBrush subBrush(Gdiplus::Color(subAlpha, subR, subG, subB));
            g.DrawString(subText.c_str(), -1, &subFont, Gdiplus::PointF(subX, contentY), &stringFormat, &subBrush);

            mainY = contentY + subBounds.Height + lineGap;
        } else if (config.showStatusDot) {
            drawStatusDot(mainX + dotSize * 0.5f, mainY + mainBounds.Height * 0.52f);
            mainX += dotSize + dotGap;
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
    if (windows_.empty()) {
        Rebuild(config);
        return;
    }

    std::wstring mainText = FormatOverlayText(config.format, config.uppercase);
    std::wstring subText = FormatOverlayText(config.subText, config.uppercase);

    for (const auto& inst : windows_) {
        if (!inst.hwnd || !IsWindow(inst.hwnd)) continue;
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
