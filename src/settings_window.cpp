#include "settings_window.h"

#include <commctrl.h>
#include <commdlg.h>
#include <cmath>
#include <cwchar>
#include <string>

namespace {

constexpr const wchar_t* kSettingsClassName = L"OverlayLabelSettingsClass";

constexpr int IDC_BTN_PRESET_CYBER = 2000;
constexpr int IDC_BTN_PRESET_GOLD = 2001;
constexpr int IDC_BTN_PRESET_RED = 2002;
constexpr int IDC_BTN_PRESET_MATRIX = 2003;
constexpr int IDC_BTN_PRESET_GLASS = 2004;
constexpr int IDC_BTN_PRESET_MINIMAL = 2005;

constexpr int IDC_EDIT_FORMAT = 2010;
constexpr int IDC_EDIT_SUBTEXT = 2011;
constexpr int IDC_COMBO_POS = 2012;
constexpr int IDC_SLIDER_MX = 2013;
constexpr int IDC_VAL_MX = 2014;
constexpr int IDC_SLIDER_MY = 2015;
constexpr int IDC_VAL_MY = 2016;
constexpr int IDC_CHK_ALLMON = 2017;
constexpr int IDC_CHK_TASKBAR = 2018;
constexpr int IDC_CHK_AUTOSTART = 2019;

constexpr int IDC_COMBO_FRAME = 2020;
constexpr int IDC_CHK_UPPER = 2021;
constexpr int IDC_EDIT_FONT = 2023;
constexpr int IDC_BTN_FONT = 2024;
constexpr int IDC_EDIT_LOGO = 2042;
constexpr int IDC_BTN_LOGO = 2043;
constexpr int IDC_BTN_CLEAR_LOGO = 2044;
constexpr int IDC_SLIDER_FONTSIZE = 2025;
constexpr int IDC_VAL_FONTSIZE = 2026;
constexpr int IDC_BTN_TEXTCOLOR = 2027;
constexpr int IDC_CHK_BOLD = 2028;
constexpr int IDC_BTN_ACCENTCOLOR = 2029;
constexpr int IDC_SLIDER_OPACITY = 2030;
constexpr int IDC_VAL_OPACITY = 2031;
constexpr int IDC_CHK_SHADOW = 2032;
constexpr int IDC_BTN_SHADOWCOLOR = 2033;
constexpr int IDC_BTN_BGCOLOR = 2034;
constexpr int IDC_HINT_PREVIEW = 2037;

constexpr int IDC_BTN_RESET = 2040;
constexpr int IDC_BTN_OPEN_PRINT = 2041;
constexpr int IDC_BTN_SAVE = IDOK;
constexpr int IDC_BTN_CANCEL = IDCANCEL;
constexpr UINT_PTR IDT_AUTOSAVE = 100;

UINT GetPrimaryDpi() {
    HDC hdc = GetDC(nullptr);
    UINT dpi = hdc ? static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX)) : 96;
    if (hdc) ReleaseDC(nullptr, hdc);
    return dpi >= 72 ? dpi : 96;
}

int PositionToIndex(OverlayPosition pos) {
    switch (pos) {
        case OverlayPosition::TopRight: return 0;
        case OverlayPosition::TopLeft: return 1;
        case OverlayPosition::BottomRight: return 2;
        case OverlayPosition::BottomLeft: return 3;
    }
    return 0;
}

OverlayPosition IndexToPosition(int idx) {
    switch (idx) {
        case 1: return OverlayPosition::TopLeft;
        case 2: return OverlayPosition::BottomRight;
        case 3: return OverlayPosition::BottomLeft;
        case 0:
        default:
            return OverlayPosition::TopRight;
    }
}

int FrameStyleToIndex(FrameStyle style) {
    switch (style) {
        case FrameStyle::Cyber: return 0;
        case FrameStyle::Pill: return 1;
        case FrameStyle::None: return 2;
    }
    return 0;
}

FrameStyle IndexToFrameStyle(int idx) {
    switch (idx) {
        case 1: return FrameStyle::Pill;
        case 2: return FrameStyle::None;
        case 0:
        default:
            return FrameStyle::Cyber;
    }
}

std::wstring FormatHex(COLORREF c) {
    wchar_t buf[16] = {};
    std::swprintf(buf, 16, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
    return buf;
}

} // namespace

SettingsWindow::~SettingsWindow() {
    Close();
    if (classRegistered_ && hInstance_) {
        UnregisterClassW(kSettingsClassName, hInstance_);
        classRegistered_ = false;
    }
}

int SettingsWindow::S(int px) const {
    return MulDiv(px, static_cast<int>(dpi_), 96);
}

void SettingsWindow::Open(HINSTANCE hInstance,
                          HICON hAppIcon,
                          const OverlayConfig& currentConfig,
                          bool currentAutoStart,
                          PreviewCallback onPreview,
                          SaveCallback onSave,
                          CloseCallback onClosed,
                          PrintCallback onOpenPrint) {
    if (hwnd_ && IsWindow(hwnd_)) {
        ShowWindow(hwnd_, SW_RESTORE);
        SetForegroundWindow(hwnd_);
        return;
    }

    hInstance_ = hInstance;
    originalConfig_ = currentConfig;
    originalAutoStart_ = currentAutoStart;
    draftConfig_ = currentConfig;
    draftAutoStart_ = currentAutoStart;
    onPreview_ = std::move(onPreview);
    onSave_ = std::move(onSave);
    onClosed_ = std::move(onClosed);
    onOpenPrint_ = std::move(onOpenPrint);
    cancelled_ = false;
    dirty_ = false;
    dpi_ = GetPrimaryDpi();

    ACTCTXW actCtx = {};
    actCtx.cbSize = sizeof(actCtx);
    actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
    actCtx.hModule = hInstance_;
    actCtx.lpResourceName = MAKEINTRESOURCEW(2);

    HANDLE hActCtx = CreateActCtxW(&actCtx);
    ULONG_PTR actCookie = 0;
    bool actActivated = false;
    if (hActCtx != INVALID_HANDLE_VALUE) {
        actActivated = (ActivateActCtx(hActCtx, &actCookie) != FALSE);
    }

    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    if (!classRegistered_) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance_;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
        wc.hIcon = hAppIcon;
        wc.hIconSm = hAppIcon;
        wc.lpszClassName = kSettingsClassName;

        if (RegisterClassExW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            classRegistered_ = true;
        } else {
            if (actActivated) DeactivateActCtx(0, actCookie);
            if (hActCtx != INVALID_HANDLE_VALUE) ReleaseActCtx(hActCtx);
            return;
        }
    }

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0, S(484), S(704) };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    RECT workArea = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int posX = workArea.left + ((workArea.right - workArea.left) - winW) / 2;
    int posY = workArea.top + ((workArea.bottom - workArea.top) - winH) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        kSettingsClassName,
        L"C\u00E0i \u0111\u1EB7t Overlay Label",
        style,
        posX, posY, winW, winH,
        nullptr,
        nullptr,
        hInstance_,
        this);

    if (actActivated) DeactivateActCtx(0, actCookie);
    if (hActCtx != INVALID_HANDLE_VALUE) ReleaseActCtx(hActCtx);

    if (!hwnd_) {
        return;
    }

    if (hAppIcon) {
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hAppIcon));
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hAppIcon));
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
}

void SettingsWindow::Close() {
    if (hwnd_ && IsWindow(hwnd_)) {
        DestroyWindow(hwnd_);
    }
    hwnd_ = nullptr;
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<SettingsWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void SettingsWindow::CreateControls() {
    hFont_ = CreateFontW(
        -MulDiv(9, static_cast<int>(dpi_), 72),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    hBoldFont_ = CreateFontW(
        -MulDiv(9, static_cast<int>(dpi_), 72),
        0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    auto addCtrl = [&](DWORD exStyle, const wchar_t* cls, const wchar_t* text, DWORD style,
                       int x, int y, int w, int h, int id, bool bold = false) -> HWND {
        HWND ctrl = CreateWindowExW(
            exStyle, cls, text, WS_CHILD | WS_VISIBLE | style,
            S(x), S(y), S(w), S(h),
            hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance_, nullptr);
        if (ctrl) {
            SendMessageW(ctrl, WM_SETFONT, reinterpret_cast<WPARAM>(bold ? hBoldFont_ : hFont_), TRUE);
        }
        return ctrl;
    };

    // --- Group 0: Quick Templates (2 rows x 3 presets) ---
    addCtrl(0, L"BUTTON", L"M\u1EABu giao di\u1EC7n (Template)", BS_GROUPBOX, 14, 10, 456, 96, 0, true);
    addCtrl(0, L"BUTTON", GetPresetDisplayName(StylePreset::CyberNet), WS_TABSTOP | BS_PUSHBUTTON, 28, 34, 136, 28, IDC_BTN_PRESET_CYBER);
    addCtrl(0, L"BUTTON", GetPresetDisplayName(StylePreset::VipGold), WS_TABSTOP | BS_PUSHBUTTON, 174, 34, 136, 28, IDC_BTN_PRESET_GOLD);
    addCtrl(0, L"BUTTON", GetPresetDisplayName(StylePreset::EsportsRed), WS_TABSTOP | BS_PUSHBUTTON, 320, 34, 136, 28, IDC_BTN_PRESET_RED);
    addCtrl(0, L"BUTTON", GetPresetDisplayName(StylePreset::MatrixGreen), WS_TABSTOP | BS_PUSHBUTTON, 28, 68, 136, 28, IDC_BTN_PRESET_MATRIX);
    addCtrl(0, L"BUTTON", GetPresetDisplayName(StylePreset::GlassModern), WS_TABSTOP | BS_PUSHBUTTON, 174, 68, 136, 28, IDC_BTN_PRESET_GLASS);
    addCtrl(0, L"BUTTON", GetPresetDisplayName(StylePreset::MinimalText), WS_TABSTOP | BS_PUSHBUTTON, 320, 68, 136, 28, IDC_BTN_PRESET_MINIMAL);

    // --- Group 1: Display ---
    addCtrl(0, L"BUTTON", L"N\u1ED9i dung & V\u1ECB tr\u00ED", BS_GROUPBOX, 14, 116, 456, 246, 0, true);

    addCtrl(0, L"STATIC", L"D\u00F2ng ch\u00EDnh:", SS_LEFT, 28, 144, 92, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 126, 140, 330, 24, IDC_EDIT_FORMAT);

    addCtrl(0, L"STATIC", L"D\u00F2ng ph\u1EE5:", SS_LEFT, 28, 174, 92, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 126, 170, 330, 24, IDC_EDIT_SUBTEXT);

    addCtrl(0, L"STATIC",
            L"Bi\u1EBFn: {username}, {computer}, {time}, {date} (tr\u1ED1ng = \u1EA9n)",
            SS_LEFTNOWORDWRAP, 126, 198, 336, 18, IDC_HINT_PREVIEW);

    addCtrl(0, L"STATIC", L"V\u1ECB tr\u00ED:", SS_LEFT, 28, 228, 92, 20, 0);
    HWND hCombo = addCtrl(0, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 126, 224, 330, 140, IDC_COMBO_POS);
    SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"G\u00F3c tr\u00EAn b\u00EAn ph\u1EA3i"));
    SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"G\u00F3c tr\u00EAn b\u00EAn tr\u00E1i"));
    SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"G\u00F3c d\u01B0\u1EDBi b\u00EAn ph\u1EA3i"));
    SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"G\u00F3c d\u01B0\u1EDBi b\u00EAn tr\u00E1i"));

    addCtrl(0, L"STATIC", L"L\u1EC1 ngang (X):", SS_LEFT, 28, 262, 92, 20, 0);
    HWND hMx = addCtrl(0, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, 126, 258, 268, 26, IDC_SLIDER_MX);
    SendMessageW(hMx, TBM_SETRANGE, TRUE, MAKELONG(0, 200));
    addCtrl(0, L"STATIC", L"18 px", SS_RIGHT, 398, 262, 58, 20, IDC_VAL_MX);

    addCtrl(0, L"STATIC", L"L\u1EC1 d\u1ECDc (Y):", SS_LEFT, 28, 292, 92, 20, 0);
    HWND hMy = addCtrl(0, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, 126, 288, 268, 26, IDC_SLIDER_MY);
    SendMessageW(hMy, TBM_SETRANGE, TRUE, MAKELONG(0, 200));
    addCtrl(0, L"STATIC", L"14 px", SS_RIGHT, 398, 292, 58, 20, IDC_VAL_MY);

    addCtrl(0, L"BUTTON", L"T\u1EA5t c\u1EA3 m\u00E0n h\u00ECnh", WS_TABSTOP | BS_AUTOCHECKBOX, 28, 328, 134, 22, IDC_CHK_ALLMON);
    addCtrl(0, L"BUTTON", L"N\u00E9 Taskbar", WS_TABSTOP | BS_AUTOCHECKBOX, 174, 328, 104, 22, IDC_CHK_TASKBAR);
    addCtrl(0, L"BUTTON", L"Ch\u1EA1y c\u00F9ng Windows", WS_TABSTOP | BS_AUTOCHECKBOX, 294, 328, 162, 22, IDC_CHK_AUTOSTART);

    // --- Group 2: Style ---
    addCtrl(0, L"BUTTON", L"Giao di\u1EC7n & M\u00E0u s\u1EAFc", BS_GROUPBOX, 14, 372, 456, 276, 0, true);

    addCtrl(0, L"STATIC", L"Ki\u1EC3u khung:", SS_LEFT, 28, 400, 92, 20, 0);
    HWND hFrame = addCtrl(0, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 126, 396, 232, 120, IDC_COMBO_FRAME);
    SendMessageW(hFrame, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Cyber Qu\u00E1n n\u00E9t (V\u00E1t g\u00F3c)"));
    SendMessageW(hFrame, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Bo tr\u00F2n (Pill)"));
    SendMessageW(hFrame, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Ch\u1EC9 ch\u1EEF (Kh\u00F4ng n\u1EC1n)"));
    addCtrl(0, L"BUTTON", L"In hoa", WS_TABSTOP | BS_AUTOCHECKBOX, 370, 398, 86, 22, IDC_CHK_UPPER);

    addCtrl(0, L"STATIC", L"Font ch\u1EEF:", SS_LEFT, 28, 434, 92, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_READONLY | ES_AUTOHSCROLL, 126, 430, 224, 24, IDC_EDIT_FONT);
    addCtrl(0, L"BUTTON", L"Ch\u1ECDn font...", WS_TABSTOP | BS_PUSHBUTTON, 358, 429, 98, 26, IDC_BTN_FONT);

    addCtrl(0, L"STATIC", L"Logo (\u1EA3nh):", SS_LEFT, 28, 468, 92, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_READONLY | ES_AUTOHSCROLL, 126, 464, 178, 24, IDC_EDIT_LOGO);
    addCtrl(0, L"BUTTON", L"Ch\u1ECDn \u1EA3nh...", WS_TABSTOP | BS_PUSHBUTTON, 310, 463, 86, 26, IDC_BTN_LOGO);
    addCtrl(0, L"BUTTON", L"X\u00F3a", WS_TABSTOP | BS_PUSHBUTTON, 402, 463, 54, 26, IDC_BTN_CLEAR_LOGO);

    addCtrl(0, L"STATIC", L"K\u00EDch th\u01B0\u1EDBc:", SS_LEFT, 28, 502, 92, 20, 0);
    HWND hFs = addCtrl(0, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, 126, 498, 268, 26, IDC_SLIDER_FONTSIZE);
    SendMessageW(hFs, TBM_SETRANGE, TRUE, MAKELONG(50, 250));
    addCtrl(0, L"STATIC", L"100%", SS_RIGHT, 398, 502, 58, 20, IDC_VAL_FONTSIZE);

    addCtrl(0, L"STATIC", L"M\u00E0u ch\u1EEF:", SS_LEFT, 28, 536, 62, 20, 0);
    addCtrl(0, L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 94, 532, 78, 24, IDC_BTN_TEXTCOLOR);
    addCtrl(0, L"STATIC", L"Vi\u1EC1n Neon:", SS_LEFT, 184, 536, 66, 20, 0);
    addCtrl(0, L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 254, 532, 78, 24, IDC_BTN_ACCENTCOLOR);
    addCtrl(0, L"BUTTON", L"In \u0111\u1EADm", WS_TABSTOP | BS_AUTOCHECKBOX, 348, 534, 96, 22, IDC_CHK_BOLD);

    addCtrl(0, L"STATIC", L"M\u00E0u n\u1EC1n:", SS_LEFT, 28, 568, 62, 20, 0);
    addCtrl(0, L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 94, 564, 78, 24, IDC_BTN_BGCOLOR);
    addCtrl(0, L"BUTTON", L"\u0110\u1ED5 b\u00F3ng:", WS_TABSTOP | BS_AUTOCHECKBOX, 184, 566, 68, 22, IDC_CHK_SHADOW);
    addCtrl(0, L"BUTTON", L"", WS_TABSTOP | BS_OWNERDRAW, 254, 564, 78, 24, IDC_BTN_SHADOWCOLOR);

    addCtrl(0, L"STATIC", L"\u0110\u1ED9 trong su\u1ED1t:", SS_LEFT, 28, 602, 92, 20, 0);
    HWND hOp = addCtrl(0, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, 126, 598, 268, 26, IDC_SLIDER_OPACITY);
    SendMessageW(hOp, TBM_SETRANGE, TRUE, MAKELONG(0, 90));
    addCtrl(0, L"STATIC", L"4%", SS_RIGHT, 398, 602, 58, 20, IDC_VAL_OPACITY);

    addCtrl(0, L"STATIC",
            L"M\u1ECDi thay \u0111\u1ED5i \u0111\u01B0\u1EE3c t\u1EF1 \u0111\u1ED9ng l\u01B0u v\u00E0 xem tr\u01B0\u1EDBc tr\u1EF1c ti\u1EBFp.",
            SS_LEFTNOWORDWRAP, 28, 628, 428, 18, IDC_HINT_PREVIEW);

    // --- Bottom Buttons ---
    addCtrl(0, L"BUTTON", L"M\u1EB7c \u0111\u1ECBnh", WS_TABSTOP | BS_PUSHBUTTON, 14, 660, 94, 32, IDC_BTN_RESET);
    addCtrl(0, L"BUTTON", L"In nh\u00E3n (PDF)...", WS_TABSTOP | BS_PUSHBUTTON, 116, 660, 132, 32, IDC_BTN_OPEN_PRINT);
    addCtrl(0, L"BUTTON", L"L\u01B0u", WS_TABSTOP | BS_DEFPUSHBUTTON, 272, 660, 94, 32, IDC_BTN_SAVE);
    addCtrl(0, L"BUTTON", L"Tho\u00E1t", WS_TABSTOP | BS_PUSHBUTTON, 376, 660, 94, 32, IDC_BTN_CANCEL);
}

void SettingsWindow::SyncControlsFromDraft() {
    syncing_ = true;

    SetDlgItemTextW(hwnd_, IDC_EDIT_FORMAT, draftConfig_.format.c_str());
    SetDlgItemTextW(hwnd_, IDC_EDIT_SUBTEXT, draftConfig_.subText.c_str());
    SendDlgItemMessageW(hwnd_, IDC_COMBO_POS, CB_SETCURSEL, PositionToIndex(draftConfig_.position), 0);

    SendDlgItemMessageW(hwnd_, IDC_SLIDER_MX, TBM_SETPOS, TRUE, draftConfig_.marginX);
    SendDlgItemMessageW(hwnd_, IDC_SLIDER_MY, TBM_SETPOS, TRUE, draftConfig_.marginY);
    CheckDlgButton(hwnd_, IDC_CHK_ALLMON, draftConfig_.allMonitors ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd_, IDC_CHK_TASKBAR, draftConfig_.respectTaskbar ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd_, IDC_CHK_AUTOSTART, draftAutoStart_ ? BST_CHECKED : BST_UNCHECKED);

    SendDlgItemMessageW(hwnd_, IDC_COMBO_FRAME, CB_SETCURSEL, FrameStyleToIndex(draftConfig_.frameStyle), 0);
    CheckDlgButton(hwnd_, IDC_CHK_UPPER, draftConfig_.uppercase ? BST_CHECKED : BST_UNCHECKED);

    SendDlgItemMessageW(hwnd_, IDC_SLIDER_FONTSIZE, TBM_SETPOS, TRUE, draftConfig_.scalePercent);
    CheckDlgButton(hwnd_, IDC_CHK_BOLD, draftConfig_.bold ? BST_CHECKED : BST_UNCHECKED);

    int transparencyPct = 100 - static_cast<int>(std::round(static_cast<double>(draftConfig_.opacity) * 100.0 / 255.0));
    if (transparencyPct < 0) transparencyPct = 0;
    if (transparencyPct > 90) transparencyPct = 90;
    SendDlgItemMessageW(hwnd_, IDC_SLIDER_OPACITY, TBM_SETPOS, TRUE, transparencyPct);

    CheckDlgButton(hwnd_, IDC_CHK_SHADOW, draftConfig_.shadow ? BST_CHECKED : BST_UNCHECKED);

    UpdateValueLabels();
    UpdateEnabledStates();

    InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_TEXTCOLOR), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_ACCENTCOLOR), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_SHADOWCOLOR), nullptr, TRUE);
    InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_BGCOLOR), nullptr, TRUE);

    syncing_ = false;
}

void SettingsWindow::UpdateValueLabels() {
    wchar_t buf[64] = {};

    std::swprintf(buf, 64, L"%d px", draftConfig_.marginX);
    SetDlgItemTextW(hwnd_, IDC_VAL_MX, buf);

    std::swprintf(buf, 64, L"%d px", draftConfig_.marginY);
    SetDlgItemTextW(hwnd_, IDC_VAL_MY, buf);

    std::wstring fontDesc = draftConfig_.fontName + L", " + std::to_wstring(draftConfig_.fontSize) + L" pt";
    if (draftConfig_.bold) {
        fontDesc += L" (\u0110\u1EADm)";
    }
    SetDlgItemTextW(hwnd_, IDC_EDIT_FONT, fontDesc.c_str());

    if (draftConfig_.logoPath.empty()) {
        SetDlgItemTextW(hwnd_, IDC_EDIT_LOGO, L"(Kh\u00F4ng c\u00F3)");
    } else {
        size_t slash = draftConfig_.logoPath.find_last_of(L"\\/");
        std::wstring fileName = (slash == std::wstring::npos)
            ? draftConfig_.logoPath
            : draftConfig_.logoPath.substr(slash + 1);
        SetDlgItemTextW(hwnd_, IDC_EDIT_LOGO, fileName.c_str());
    }

    std::swprintf(buf, 64, L"%d%%", draftConfig_.scalePercent);
    SetDlgItemTextW(hwnd_, IDC_VAL_FONTSIZE, buf);

    int transparencyPct = 100 - static_cast<int>(std::round(static_cast<double>(draftConfig_.opacity) * 100.0 / 255.0));
    if (transparencyPct < 0) transparencyPct = 0;
    if (transparencyPct > 90) transparencyPct = 90;
    std::swprintf(buf, 64, L"%d%%", transparencyPct);
    SetDlgItemTextW(hwnd_, IDC_VAL_OPACITY, buf);
}

void SettingsWindow::UpdateEnabledStates() {
    bool hasBg = (draftConfig_.frameStyle != FrameStyle::None);
    EnableWindow(GetDlgItem(hwnd_, IDC_BTN_SHADOWCOLOR), draftConfig_.shadow ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwnd_, IDC_BTN_BGCOLOR), hasBg ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwnd_, IDC_BTN_ACCENTCOLOR), (hasBg || !draftConfig_.subText.empty()) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwnd_, IDC_BTN_CLEAR_LOGO), !draftConfig_.logoPath.empty() ? TRUE : FALSE);
}

void SettingsWindow::SyncPositionFromExternal(OverlayPosition pos, int marginX, int marginY) {
    draftConfig_.position = pos;
    draftConfig_.marginX = marginX;
    draftConfig_.marginY = marginY;

    if (!hwnd_ || !IsWindow(hwnd_)) return;

    syncing_ = true;
    SendDlgItemMessageW(hwnd_, IDC_COMBO_POS, CB_SETCURSEL, PositionToIndex(draftConfig_.position), 0);
    int sliderX = marginX > 200 ? 200 : (marginX < 0 ? 0 : marginX);
    int sliderY = marginY > 200 ? 200 : (marginY < 0 ? 0 : marginY);
    SendDlgItemMessageW(hwnd_, IDC_SLIDER_MX, TBM_SETPOS, TRUE, sliderX);
    SendDlgItemMessageW(hwnd_, IDC_SLIDER_MY, TBM_SETPOS, TRUE, sliderY);
    UpdateValueLabels();
    syncing_ = false;
}

void SettingsWindow::NotifyPreview(bool immediateSave) {
    if (onPreview_) {
        onPreview_(draftConfig_);
    }
    if (immediateSave) {
        dirty_ = true;
        FlushAutoSave();
    } else {
        ScheduleAutoSave();
    }
}

void SettingsWindow::ScheduleAutoSave() {
    dirty_ = true;
    if (hwnd_ && IsWindow(hwnd_)) {
        SetTimer(hwnd_, IDT_AUTOSAVE, 180, nullptr);
    } else {
        FlushAutoSave();
    }
}

void SettingsWindow::FlushAutoSave() {
    if (hwnd_ && IsWindow(hwnd_)) {
        KillTimer(hwnd_, IDT_AUTOSAVE);
    }
    if (dirty_ && onSave_) {
        dirty_ = false;
        onSave_(draftConfig_, draftAutoStart_);
    }
}

bool SettingsWindow::PickColor(COLORREF& color) {
    CHOOSECOLORW cc = {};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd_;
    cc.lpCustColors = customColors_;
    cc.rgbResult = color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorW(&cc)) {
        color = cc.rgbResult;
        return true;
    }
    return false;
}

void SettingsWindow::PickFont() {
    LOGFONTW lf = {};
    lf.lfHeight = -MulDiv(draftConfig_.fontSize, static_cast<int>(dpi_), 72);
    lf.lfWeight = draftConfig_.bold ? FW_BOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    size_t maxLen = (sizeof(lf.lfFaceName) / sizeof(lf.lfFaceName[0])) - 1;
    size_t copyLen = draftConfig_.fontName.size() < maxLen ? draftConfig_.fontName.size() : maxLen;
    std::wmemcpy(lf.lfFaceName, draftConfig_.fontName.c_str(), copyLen);
    lf.lfFaceName[copyLen] = L'\0';

    CHOOSEFONTW cf = {};
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hwnd_;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST;
    cf.nSizeMin = 8;
    cf.nSizeMax = 72;

    if (ChooseFontW(&cf)) {
        draftConfig_.fontName = lf.lfFaceName;
        int pt = cf.iPointSize / 10;
        if (pt < 8) pt = 8;
        if (pt > 72) pt = 72;
        draftConfig_.fontSize = pt;
        draftConfig_.bold = (lf.lfWeight >= FW_SEMIBOLD);
        SyncControlsFromDraft();
        NotifyPreview();
    }
}

void SettingsWindow::PickLogo() {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.ico)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.ico\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        draftConfig_.logoPath = filePath;
        SyncControlsFromDraft();
        NotifyPreview();
    }
}

void SettingsWindow::DrawColorButton(const DRAWITEMSTRUCT* dis, COLORREF color, bool enabled) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    COLORREF fill = enabled ? color : GetSysColor(COLOR_BTNFACE);
    HBRUSH hBrush = CreateSolidBrush(fill);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 1, enabled ? RGB(110, 110, 110) : RGB(170, 170, 170));
    HGDIOBJ oldPen = SelectObject(hdc, hPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(hPen);

    std::wstring hex = FormatHex(color);
    int luma = (299 * GetRValue(fill) + 587 * GetGValue(fill) + 114 * GetBValue(fill)) / 1000;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, enabled ? (luma > 140 ? RGB(20, 20, 20) : RGB(255, 255, 255)) : RGB(130, 130, 130));
    HGDIOBJ oldFont = SelectObject(hdc, hFont_);
    DrawTextW(hdc, hex.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);

    if (dis->itemState & ODS_FOCUS) {
        RECT focusRc = rc;
        InflateRect(&focusRc, -2, -2);
        DrawFocusRect(hdc, &focusRc);
    }
}

LRESULT SettingsWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateControls();
            SyncControlsFromDraft();
            return 0;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            HWND hCtrl = reinterpret_cast<HWND>(lParam);
            SetBkMode(hdc, TRANSPARENT);
            if (GetDlgCtrlID(hCtrl) == IDC_HINT_PREVIEW) {
                SetTextColor(hdc, RGB(100, 100, 100));
            }
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }

        case WM_DRAWITEM: {
            auto* dis = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
            if (!dis) break;
            bool hasBg = (draftConfig_.frameStyle != FrameStyle::None);
            if (dis->CtlID == IDC_BTN_TEXTCOLOR) {
                DrawColorButton(dis, draftConfig_.textColor, true);
                return TRUE;
            }
            if (dis->CtlID == IDC_BTN_ACCENTCOLOR) {
                DrawColorButton(dis, draftConfig_.accentColor, hasBg || !draftConfig_.subText.empty());
                return TRUE;
            }
            if (dis->CtlID == IDC_BTN_SHADOWCOLOR) {
                DrawColorButton(dis, draftConfig_.shadowColor, draftConfig_.shadow);
                return TRUE;
            }
            if (dis->CtlID == IDC_BTN_BGCOLOR) {
                DrawColorButton(dis, draftConfig_.backgroundColor, hasBg);
                return TRUE;
            }
            break;
        }

        case WM_TIMER:
            if (wParam == IDT_AUTOSAVE) {
                FlushAutoSave();
                return 0;
            }
            break;

        case WM_HSCROLL: {
            if (syncing_) return 0;
            HWND hSlider = reinterpret_cast<HWND>(lParam);
            if (!hSlider) break;
            int id = GetDlgCtrlID(hSlider);
            int pos = static_cast<int>(SendMessageW(hSlider, TBM_GETPOS, 0, 0));

            switch (id) {
                case IDC_SLIDER_MX: draftConfig_.marginX = pos; break;
                case IDC_SLIDER_MY: draftConfig_.marginY = pos; break;
                case IDC_SLIDER_FONTSIZE: draftConfig_.scalePercent = pos; break;
                case IDC_SLIDER_OPACITY: {
                    int alpha = static_cast<int>(std::round((100.0 - static_cast<double>(pos)) * 255.0 / 100.0));
                    if (alpha < 25) alpha = 25;
                    if (alpha > 255) alpha = 255;
                    draftConfig_.opacity = static_cast<BYTE>(alpha);
                    break;
                }
                default: return 0;
            }
            UpdateValueLabels();
            WORD scrollCode = LOWORD(wParam);
            bool finished = (scrollCode == TB_ENDTRACK || scrollCode == SB_ENDSCROLL);
            NotifyPreview(finished);
            return 0;
        }

        case WM_COMMAND: {
            if (syncing_) return 0;
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            switch (id) {
                case IDC_BTN_PRESET_CYBER:
                    draftConfig_ = MakePresetConfig(StylePreset::CyberNet, draftConfig_);
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_BTN_PRESET_GOLD:
                    draftConfig_ = MakePresetConfig(StylePreset::VipGold, draftConfig_);
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_BTN_PRESET_RED:
                    draftConfig_ = MakePresetConfig(StylePreset::EsportsRed, draftConfig_);
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_BTN_PRESET_MATRIX:
                    draftConfig_ = MakePresetConfig(StylePreset::MatrixGreen, draftConfig_);
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_BTN_PRESET_GLASS:
                    draftConfig_ = MakePresetConfig(StylePreset::GlassModern, draftConfig_);
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_BTN_PRESET_MINIMAL:
                    draftConfig_ = MakePresetConfig(StylePreset::MinimalText, draftConfig_);
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_EDIT_FORMAT:
                    if (code == EN_CHANGE) {
                        wchar_t buf[256] = {};
                        GetDlgItemTextW(hwnd_, IDC_EDIT_FORMAT, buf, 256);
                        draftConfig_.format = buf;
                        NotifyPreview(false);
                    }
                    return 0;

                case IDC_EDIT_SUBTEXT:
                    if (code == EN_CHANGE) {
                        wchar_t buf[256] = {};
                        GetDlgItemTextW(hwnd_, IDC_EDIT_SUBTEXT, buf, 256);
                        draftConfig_.subText = buf;
                        UpdateEnabledStates();
                        InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_ACCENTCOLOR), nullptr, TRUE);
                        NotifyPreview(false);
                    }
                    return 0;

                case IDC_COMBO_POS:
                    if (code == CBN_SELCHANGE) {
                        int sel = static_cast<int>(SendDlgItemMessageW(hwnd_, IDC_COMBO_POS, CB_GETCURSEL, 0, 0));
                        draftConfig_.position = IndexToPosition(sel);
                        NotifyPreview();
                    }
                    return 0;

                case IDC_COMBO_FRAME:
                    if (code == CBN_SELCHANGE) {
                        int sel = static_cast<int>(SendDlgItemMessageW(hwnd_, IDC_COMBO_FRAME, CB_GETCURSEL, 0, 0));
                        draftConfig_.frameStyle = IndexToFrameStyle(sel);
                        draftConfig_.background = (draftConfig_.frameStyle != FrameStyle::None);
                        UpdateEnabledStates();
                        InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_BGCOLOR), nullptr, TRUE);
                        InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_ACCENTCOLOR), nullptr, TRUE);
                        NotifyPreview();
                    }
                    return 0;

                case IDC_CHK_UPPER:
                    draftConfig_.uppercase = (IsDlgButtonChecked(hwnd_, IDC_CHK_UPPER) == BST_CHECKED);
                    NotifyPreview();
                    return 0;

                case IDC_BTN_LOGO:
                    PickLogo();
                    return 0;

                case IDC_BTN_CLEAR_LOGO:
                    draftConfig_.logoPath.clear();
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_CHK_ALLMON:
                    draftConfig_.allMonitors = (IsDlgButtonChecked(hwnd_, IDC_CHK_ALLMON) == BST_CHECKED);
                    NotifyPreview();
                    return 0;

                case IDC_CHK_TASKBAR:
                    draftConfig_.respectTaskbar = (IsDlgButtonChecked(hwnd_, IDC_CHK_TASKBAR) == BST_CHECKED);
                    NotifyPreview();
                    return 0;

                case IDC_CHK_AUTOSTART:
                    draftAutoStart_ = (IsDlgButtonChecked(hwnd_, IDC_CHK_AUTOSTART) == BST_CHECKED);
                    dirty_ = true;
                    FlushAutoSave();
                    return 0;

                case IDC_BTN_FONT:
                    PickFont();
                    return 0;

                case IDC_CHK_BOLD:
                    draftConfig_.bold = (IsDlgButtonChecked(hwnd_, IDC_CHK_BOLD) == BST_CHECKED);
                    UpdateValueLabels();
                    NotifyPreview();
                    return 0;

                case IDC_BTN_TEXTCOLOR:
                    if (PickColor(draftConfig_.textColor)) {
                        InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_TEXTCOLOR), nullptr, TRUE);
                        NotifyPreview();
                    }
                    return 0;

                case IDC_BTN_ACCENTCOLOR:
                    if (PickColor(draftConfig_.accentColor)) {
                        InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_ACCENTCOLOR), nullptr, TRUE);
                        NotifyPreview();
                    }
                    return 0;

                case IDC_CHK_SHADOW:
                    draftConfig_.shadow = (IsDlgButtonChecked(hwnd_, IDC_CHK_SHADOW) == BST_CHECKED);
                    UpdateEnabledStates();
                    InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_SHADOWCOLOR), nullptr, TRUE);
                    NotifyPreview();
                    return 0;

                case IDC_BTN_SHADOWCOLOR:
                    if (PickColor(draftConfig_.shadowColor)) {
                        InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_SHADOWCOLOR), nullptr, TRUE);
                        NotifyPreview();
                    }
                    return 0;

                case IDC_BTN_BGCOLOR:
                    if (PickColor(draftConfig_.backgroundColor)) {
                        InvalidateRect(GetDlgItem(hwnd_, IDC_BTN_BGCOLOR), nullptr, TRUE);
                        NotifyPreview();
                    }
                    return 0;

                case IDC_BTN_RESET:
                    draftConfig_ = OverlayConfig{};
                    SyncControlsFromDraft();
                    NotifyPreview();
                    return 0;

                case IDC_BTN_OPEN_PRINT:
                    FlushAutoSave();
                    if (onOpenPrint_) {
                        onOpenPrint_();
                    }
                    return 0;

                case IDC_BTN_SAVE:
                case IDC_BTN_CANCEL:
                    FlushAutoSave();
                    DestroyWindow(hwnd_);
                    return 0;
            }
            break;
        }

        case WM_CLOSE:
            FlushAutoSave();
            DestroyWindow(hwnd_);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd_, IDT_AUTOSAVE);
            if (!cancelled_) {
                FlushAutoSave();
            }
            if (hFont_) {
                DeleteObject(hFont_);
                hFont_ = nullptr;
            }
            if (hBoldFont_) {
                DeleteObject(hBoldFont_);
                hBoldFont_ = nullptr;
            }
            hwnd_ = nullptr;
            if (onClosed_) {
                auto cb = std::move(onClosed_);
                cb();
            }
            return 0;
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}
