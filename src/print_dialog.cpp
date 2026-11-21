#include "print_dialog.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <cwchar>
#include <cwctype>
#include <string>

namespace {

constexpr const wchar_t* kPrintDialogClassName = L"OverlayLabelPrintDialogClass";
constexpr const wchar_t* kA4PreviewClassName = L"OverlayLabelA4PreviewClass";

constexpr int IDC_PR_PREFIX = 3001;
constexpr int IDC_PR_SEP = 3002;
constexpr int IDC_PR_FROM = 3003;
constexpr int IDC_PR_TO = 3004;
constexpr int IDC_PR_PADZERO = 3005;
constexpr int IDC_PR_SUB = 3006;
constexpr int IDC_PR_LAYOUT = 3007;
constexpr int IDC_PR_CUTLINES = 3008;
constexpr int IDC_PR_PREVIEW = 3009;
constexpr int IDC_PR_SUMMARY = 3010;
constexpr int IDC_PR_OPEN_A4_PREVIEW = 3011;
constexpr int IDC_PR_PRINT = 3012;
constexpr int IDC_PR_EXPORT_PDF = IDOK;
constexpr int IDC_PR_CLOSE = IDCANCEL;

// Controls inside Full A4 Preview Window
constexpr int IDC_PV_PREV = 3101;
constexpr int IDC_PV_NEXT = 3102;
constexpr int IDC_PV_PAGE_LABEL = 3103;
constexpr int IDC_PV_PRINT = 3104;
constexpr int IDC_PV_EXPORT = 3105;
constexpr int IDC_PV_CLOSE = 3106;

UINT GetPrimaryScreenDpi() {
    HDC hdc = GetDC(nullptr);
    UINT dpi = hdc ? static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX)) : 96;
    if (hdc) ReleaseDC(nullptr, hdc);
    return dpi >= 72 ? dpi : 96;
}

std::wstring SanitizeFilePart(const std::wstring& s) {
    std::wstring out;
    for (wchar_t ch : s) {
        if (std::iswalnum(ch) || ch == L'-' || ch == L'_') {
            out.push_back(ch);
        }
    }
    return out.empty() ? L"Label" : out;
}

} // namespace

PrintDialog::~PrintDialog() {
    Close();
    if (previewClassRegistered_ && hInstance_) {
        UnregisterClassW(kA4PreviewClassName, hInstance_);
        previewClassRegistered_ = false;
    }
    if (classRegistered_ && hInstance_) {
        UnregisterClassW(kPrintDialogClassName, hInstance_);
        classRegistered_ = false;
    }
}

int PrintDialog::S(int px) const {
    return MulDiv(px, static_cast<int>(dpi_), 96);
}

int PrintDialog::GetTotalPages() const {
    int low = job_.startNum <= job_.endNum ? job_.startNum : job_.endNum;
    int high = job_.startNum >= job_.endNum ? job_.startNum : job_.endNum;
    int count = (high - low) + 1;
    if (count < 1) count = 1;
    int cols = 2, rows = 4;
    GetPrintLayoutGrid(job_.layoutIndex, cols, rows);
    int perPage = cols * rows;
    return (count + perPage - 1) / perPage;
}

void PrintDialog::Open(HINSTANCE hInstance, HICON hAppIcon, const OverlayConfig& currentStyle) {
    style_ = currentStyle;
    hAppIcon_ = hAppIcon;

    if (hwnd_ && IsWindow(hwnd_)) {
        UpdateSummaryAndPreview();
        ShowWindow(hwnd_, SW_RESTORE);
        SetForegroundWindow(hwnd_);
        return;
    }

    hInstance_ = hInstance;
    dpi_ = GetPrimaryScreenDpi();

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
    icex.dwICC = ICC_STANDARD_CLASSES;
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
        wc.lpszClassName = kPrintDialogClassName;

        if (RegisterClassExW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            classRegistered_ = true;
        } else {
            if (actActivated) DeactivateActCtx(0, actCookie);
            if (hActCtx != INVALID_HANDLE_VALUE) ReleaseActCtx(hActCtx);
            return;
        }
    }

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc = { 0, 0, S(484), S(488) };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    RECT workArea = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int posX = workArea.left + ((workArea.right - workArea.left) - winW) / 2;
    int posY = workArea.top + ((workArea.bottom - workArea.top) - winH) / 2;

    hwnd_ = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        kPrintDialogClassName,
        L"In nh\u00E3n ph\u00F2ng m\u00E1y (PDF) \u2014 Overlay Label",
        style,
        posX, posY, winW, winH,
        nullptr,
        nullptr,
        hInstance_,
        this);

    if (actActivated) DeactivateActCtx(0, actCookie);
    if (hActCtx != INVALID_HANDLE_VALUE) ReleaseActCtx(hActCtx);

    if (!hwnd_) return;

    if (hAppIcon) {
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hAppIcon));
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hAppIcon));
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
}

void PrintDialog::UpdateStyle(const OverlayConfig& currentStyle) {
    style_ = currentStyle;
    if (hwnd_ && IsWindow(hwnd_)) {
        UpdateSummaryAndPreview();
    }
    if (previewHwnd_ && IsWindow(previewHwnd_)) {
        InvalidateRect(previewHwnd_, nullptr, FALSE);
    }
}

void PrintDialog::Close() {
    if (previewHwnd_ && IsWindow(previewHwnd_)) {
        DestroyWindow(previewHwnd_);
    }
    previewHwnd_ = nullptr;
    if (hwnd_ && IsWindow(hwnd_)) {
        DestroyWindow(hwnd_);
    }
    hwnd_ = nullptr;
}

LRESULT CALLBACK PrintDialog::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PrintDialog* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<PrintDialog*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<PrintDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PrintDialog::PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PrintDialog* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<PrintDialog*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->previewHwnd_ = hwnd;
    } else {
        self = reinterpret_cast<PrintDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandlePreviewMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void PrintDialog::CreateControls() {
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

    // --- Group 1: Label Content ---
    addCtrl(0, L"BUTTON", L"N\u1ED9i dung nh\u00E3n h\u00E0ng lo\u1EA1t", BS_GROUPBOX, 14, 10, 456, 142, 0, true);

    addCtrl(0, L"STATIC", L"Ti\u1EC1n t\u1ED1 (\u00D4 1):", SS_LEFT, 28, 40, 94, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 126, 36, 174, 24, IDC_PR_PREFIX);
    addCtrl(0, L"STATIC", L"D\u1EA5u n\u1ED1i:", SS_LEFT, 312, 40, 56, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 372, 36, 84, 24, IDC_PR_SEP);

    addCtrl(0, L"STATIC", L"S\u1ED1 (\u00D4 2) t\u1EEB:", SS_LEFT, 28, 74, 94, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_NUMBER, 126, 70, 72, 24, IDC_PR_FROM);
    addCtrl(0, L"STATIC", L"\u0111\u1EBFn:", SS_CENTER, 202, 74, 34, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_NUMBER, 240, 70, 72, 24, IDC_PR_TO);
    addCtrl(0, L"BUTTON", L"\u0110\u1EC7m 0 (01, 02...)", WS_TABSTOP | BS_AUTOCHECKBOX, 324, 72, 134, 22, IDC_PR_PADZERO);

    addCtrl(0, L"STATIC", L"D\u00F2ng ph\u1EE5:", SS_LEFT, 28, 110, 94, 20, 0);
    addCtrl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 126, 106, 330, 24, IDC_PR_SUB);

    // --- Group 2: A4 Page Layout ---
    addCtrl(0, L"BUTTON", L"D\u00E0n trang kh\u1ED5 A4 (chu\u1EA9n ph\u00F2ng m\u00E1y nh\u00ECn t\u1EEB xa)", BS_GROUPBOX, 14, 162, 456, 96, 0, true);

    addCtrl(0, L"STATIC", L"K\u00EDch th\u01B0\u1EDBc:", SS_LEFT, 28, 192, 94, 20, 0);
    HWND hLayout = addCtrl(0, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 126, 188, 330, 140, IDC_PR_LAYOUT);
    SendMessageW(hLayout, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"C\u1EE1 L\u1EDBn \u2014 2\u00D74 (8 nh\u00E3n/A4 \u00B7 ~9\u00D76.5 cm \u00B7 Nh\u00ECn xa)"));
    SendMessageW(hLayout, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"C\u1EE1 \u0110\u1EA1i \u2014 1\u00D73 (3 nh\u00E3n/A4 \u00B7 ~18\u00D78.5 cm \u00B7 Si\u00EAu to)"));
    SendMessageW(hLayout, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"C\u1EE1 V\u1EEBa \u2014 2\u00D75 (10 nh\u00E3n/A4 \u00B7 ~9\u00D75.2 cm \u00B7 D\u00E1n m\u00E0n h\u00ECnh)"));
    SendMessageW(hLayout, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"C\u1EE1 G\u1ECDn \u2014 3\u00D76 (18 nh\u00E3n/A4 \u00B7 ~6\u00D74.2 cm \u00B7 Ti\u1EBFt ki\u1EC7m)"));

    addCtrl(0, L"BUTTON",
            L"K\u1EBB \u0111\u01B0\u1EDDng vi\u1EC1n c\u1EAFt \u0111\u1EE9t n\u00E9t quanh t\u1EEBng nh\u00E3n",
            WS_TABSTOP | BS_AUTOCHECKBOX, 126, 224, 330, 22, IDC_PR_CUTLINES);

    // --- Group 3: Live Card Preview ---
    addCtrl(0, L"BUTTON", L"Xem tr\u01B0\u1EDBc m\u1EABu nh\u00E3n (l\u1EA5y m\u00E0u theo Overlay hi\u1EC7n t\u1EA1i)", BS_GROUPBOX, 14, 268, 456, 162, 0, true);
    addCtrl(WS_EX_CLIENTEDGE, L"STATIC", L"", SS_OWNERDRAW, 28, 292, 428, 106, IDC_PR_PREVIEW);
    addCtrl(0, L"STATIC", L"", SS_LEFTNOWORDWRAP, 28, 405, 428, 18, IDC_PR_SUMMARY);

    // --- Bottom Buttons ---
    addCtrl(0, L"BUTTON", L"Xem tr\u01B0\u1EDBc A4...", WS_TABSTOP | BS_PUSHBUTTON, 14, 442, 122, 32, IDC_PR_OPEN_A4_PREVIEW);
    addCtrl(0, L"BUTTON", L"In tr\u1EF1c ti\u1EBFp...", WS_TABSTOP | BS_PUSHBUTTON, 144, 442, 104, 32, IDC_PR_PRINT);
    addCtrl(0, L"BUTTON", L"Xu\u1EA5t file PDF...", WS_TABSTOP | BS_DEFPUSHBUTTON, 256, 442, 126, 32, IDC_PR_EXPORT_PDF);
    addCtrl(0, L"BUTTON", L"\u0110\u00F3ng", WS_TABSTOP | BS_PUSHBUTTON, 390, 442, 80, 32, IDC_PR_CLOSE);
}

void PrintDialog::SyncControlsFromJob() {
    syncing_ = true;
    SetDlgItemTextW(hwnd_, IDC_PR_PREFIX, job_.prefix.c_str());
    SetDlgItemTextW(hwnd_, IDC_PR_SEP, job_.separator.c_str());
    SetDlgItemInt(hwnd_, IDC_PR_FROM, static_cast<UINT>(job_.startNum), FALSE);
    SetDlgItemInt(hwnd_, IDC_PR_TO, static_cast<UINT>(job_.endNum), FALSE);
    CheckDlgButton(hwnd_, IDC_PR_PADZERO, job_.padZero ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextW(hwnd_, IDC_PR_SUB, job_.subLabel.c_str());
    SendDlgItemMessageW(hwnd_, IDC_PR_LAYOUT, CB_SETCURSEL, job_.layoutIndex, 0);
    CheckDlgButton(hwnd_, IDC_PR_CUTLINES, job_.showCutLines ? BST_CHECKED : BST_UNCHECKED);
    syncing_ = false;
    UpdateSummaryAndPreview();
}

void PrintDialog::ReadJobFromControls() {
    wchar_t buf[256] = {};

    GetDlgItemTextW(hwnd_, IDC_PR_PREFIX, buf, 256);
    job_.prefix = buf;

    GetDlgItemTextW(hwnd_, IDC_PR_SEP, buf, 64);
    job_.separator = buf;

    BOOL okFrom = FALSE;
    UINT vFrom = GetDlgItemInt(hwnd_, IDC_PR_FROM, &okFrom, FALSE);
    job_.startNum = okFrom ? static_cast<int>(vFrom) : 1;

    BOOL okTo = FALSE;
    UINT vTo = GetDlgItemInt(hwnd_, IDC_PR_TO, &okTo, FALSE);
    job_.endNum = okTo ? static_cast<int>(vTo) : job_.startNum;

    if (job_.startNum > 9999) job_.startNum = 9999;
    if (job_.endNum > 9999) job_.endNum = 9999;

    job_.padZero = (IsDlgButtonChecked(hwnd_, IDC_PR_PADZERO) == BST_CHECKED);

    GetDlgItemTextW(hwnd_, IDC_PR_SUB, buf, 256);
    job_.subLabel = buf;

    int sel = static_cast<int>(SendDlgItemMessageW(hwnd_, IDC_PR_LAYOUT, CB_GETCURSEL, 0, 0));
    job_.layoutIndex = (sel >= 0 && sel <= 3) ? sel : 0;

    job_.showCutLines = (IsDlgButtonChecked(hwnd_, IDC_PR_CUTLINES) == BST_CHECKED);
}

void PrintDialog::UpdateSummaryAndPreview() {
    int low = job_.startNum <= job_.endNum ? job_.startNum : job_.endNum;
    int high = job_.startNum >= job_.endNum ? job_.startNum : job_.endNum;
    int count = (high - low) + 1;
    int pages = GetTotalPages();

    std::wstring firstLbl = FormatPrintLabelNumber(job_, low, style_.uppercase);
    std::wstring lastLbl = FormatPrintLabelNumber(job_, high, style_.uppercase);

    std::wstring summary = L"D\u1EF1 ki\u1EBFn: " + firstLbl + L" \u2192 " + lastLbl +
                           L"  (" + std::to_wstring(count) + L" nh\u00E3n \u00B7 " +
                           std::to_wstring(pages) + L" trang A4)";
    SetDlgItemTextW(hwnd_, IDC_PR_SUMMARY, summary.c_str());

    InvalidateRect(GetDlgItem(hwnd_, IDC_PR_PREVIEW), nullptr, TRUE);

    if (previewHwnd_ && IsWindow(previewHwnd_)) {
        if (previewPage_ >= pages) previewPage_ = pages - 1;
        if (previewPage_ < 0) previewPage_ = 0;
        UpdateA4PreviewNav();
        InvalidateRect(previewHwnd_, nullptr, FALSE);
    }
}

void PrintDialog::OpenA4PreviewWindow() {
    ReadJobFromControls();
    previewPage_ = 0;

    if (previewHwnd_ && IsWindow(previewHwnd_)) {
        UpdateA4PreviewNav();
        InvalidateRect(previewHwnd_, nullptr, FALSE);
        ShowWindow(previewHwnd_, SW_RESTORE);
        SetForegroundWindow(previewHwnd_);
        return;
    }

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

    if (!previewClassRegistered_) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = PreviewWndProc;
        wc.hInstance = hInstance_;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
        wc.hIcon = hAppIcon_;
        wc.hIconSm = hAppIcon_;
        wc.lpszClassName = kA4PreviewClassName;
        if (RegisterClassExW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            previewClassRegistered_ = true;
        }
    }

    RECT workArea = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int screenW = workArea.right - workArea.left;
    int screenH = workArea.bottom - workArea.top;

    int winH = static_cast<int>(screenH * 0.88);
    if (winH > S(840)) winH = S(840);
    if (winH < S(520)) winH = S(520);
    int winW = static_cast<int>((winH - S(90)) * (210.0 / 297.0)) + S(56);
    if (winW < S(540)) winW = S(540);
    if (winW > screenW - S(40)) winW = screenW - S(40);

    int posX = workArea.left + (screenW - winW) / 2;
    int posY = workArea.top + (screenH - winH) / 2;

    previewHwnd_ = CreateWindowExW(
        0,
        kA4PreviewClassName,
        L"Xem tr\u01B0\u1EDBc trang in A4 \u2014 Overlay Label",
        WS_OVERLAPPEDWINDOW,
        posX, posY, winW, winH,
        hwnd_,
        nullptr,
        hInstance_,
        this);

    if (actActivated) DeactivateActCtx(0, actCookie);
    if (hActCtx != INVALID_HANDLE_VALUE) ReleaseActCtx(hActCtx);

    if (!previewHwnd_) return;

    if (hAppIcon_) {
        SendMessageW(previewHwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hAppIcon_));
        SendMessageW(previewHwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hAppIcon_));
    }

    ShowWindow(previewHwnd_, SW_SHOW);
    UpdateWindow(previewHwnd_);
    SetForegroundWindow(previewHwnd_);
}

void PrintDialog::LayoutA4PreviewControls() {
    if (!previewHwnd_ || !IsWindow(previewHwnd_)) return;

    RECT rc = {};
    GetClientRect(previewHwnd_, &rc);
    int cw = rc.right - rc.left;
    int ch = rc.bottom - rc.top;
    int barH = S(46);
    int y = ch - barH + S(8);
    int btnH = S(30);

    MoveWindow(GetDlgItem(previewHwnd_, IDC_PV_PREV), S(12), y, S(86), btnH, TRUE);
    MoveWindow(GetDlgItem(previewHwnd_, IDC_PV_PAGE_LABEL), S(104), y + S(6), S(90), S(20), TRUE);
    MoveWindow(GetDlgItem(previewHwnd_, IDC_PV_NEXT), S(198), y, S(86), btnH, TRUE);

    int rightX = cw - S(12);
    MoveWindow(GetDlgItem(previewHwnd_, IDC_PV_CLOSE), rightX - S(72), y, S(72), btnH, TRUE);
    rightX -= S(80);
    MoveWindow(GetDlgItem(previewHwnd_, IDC_PV_EXPORT), rightX - S(108), y, S(108), btnH, TRUE);
    rightX -= S(116);
    MoveWindow(GetDlgItem(previewHwnd_, IDC_PV_PRINT), rightX - S(76), y, S(76), btnH, TRUE);
}

void PrintDialog::UpdateA4PreviewNav() {
    if (!previewHwnd_ || !IsWindow(previewHwnd_)) return;

    int total = GetTotalPages();
    if (previewPage_ >= total) previewPage_ = total - 1;
    if (previewPage_ < 0) previewPage_ = 0;

    std::wstring lbl = L"Trang " + std::to_wstring(previewPage_ + 1) + L" / " + std::to_wstring(total);
    SetDlgItemTextW(previewHwnd_, IDC_PV_PAGE_LABEL, lbl.c_str());

    EnableWindow(GetDlgItem(previewHwnd_, IDC_PV_PREV), previewPage_ > 0 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(previewHwnd_, IDC_PV_NEXT), (previewPage_ + 1) < total ? TRUE : FALSE);
}

LRESULT PrintDialog::HandlePreviewMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            auto addBtn = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
                HWND c = CreateWindowExW(
                    0, cls, text, WS_CHILD | WS_VISIBLE | style,
                    0, 0, 10, 10,
                    hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance_, nullptr);
                if (c) SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);
            };

            addBtn(L"BUTTON", L"\u25C0 Tr\u01B0\u1EDBc", WS_TABSTOP | BS_PUSHBUTTON, IDC_PV_PREV);
            addBtn(L"STATIC", L"Trang 1 / 1", SS_CENTER, IDC_PV_PAGE_LABEL);
            addBtn(L"BUTTON", L"Sau \u25B6", WS_TABSTOP | BS_PUSHBUTTON, IDC_PV_NEXT);
            addBtn(L"BUTTON", L"In ngay", WS_TABSTOP | BS_PUSHBUTTON, IDC_PV_PRINT);
            addBtn(L"BUTTON", L"Xu\u1EA5t PDF...", WS_TABSTOP | BS_DEFPUSHBUTTON, IDC_PV_EXPORT);
            addBtn(L"BUTTON", L"\u0110\u00F3ng", WS_TABSTOP | BS_PUSHBUTTON, IDC_PV_CLOSE);

            LayoutA4PreviewControls();
            UpdateA4PreviewNav();
            return 0;
        }

        case WM_SIZE:
            LayoutA4PreviewControls();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            if (mmi) {
                mmi->ptMinTrackSize.x = S(540);
                mmi->ptMinTrackSize.y = S(480);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }

        case WM_PAINT: {
            PAINTSTRUCT ps = {};
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRc = {};
            GetClientRect(hwnd, &clientRc);
            int barH = S(46);

            RECT canvasRc = clientRc;
            canvasRc.bottom = (clientRc.bottom > barH) ? (clientRc.bottom - barH) : 0;
            if (canvasRc.bottom > canvasRc.top) {
                RenderPrintA4PagePreview(hdc, canvasRc, style_, job_, previewPage_);
            }

            RECT barRc = clientRc;
            barRc.top = canvasRc.bottom;
            FillRect(hdc, &barRc, GetSysColorBrush(COLOR_BTNFACE));

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_PV_PREV:
                    if (previewPage_ > 0) {
                        --previewPage_;
                        UpdateA4PreviewNav();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;

                case IDC_PV_NEXT:
                    if (previewPage_ + 1 < GetTotalPages()) {
                        ++previewPage_;
                        UpdateA4PreviewNav();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;

                case IDC_PV_EXPORT:
                    HandleExportPdf();
                    return 0;

                case IDC_PV_PRINT:
                    HandleDirectPrint();
                    return 0;

                case IDC_PV_CLOSE:
                    DestroyWindow(hwnd);
                    return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            previewHwnd_ = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void PrintDialog::HandleExportPdf() {
    ReadJobFromControls();

    int low = job_.startNum <= job_.endNum ? job_.startNum : job_.endNum;
    int high = job_.startNum >= job_.endNum ? job_.startNum : job_.endNum;
    if ((high - low) + 1 > 1000) {
        MessageBoxW(
            hwnd_,
            L"T\u1ED1i \u0111a 1000 nh\u00E3n m\u1ED7i l\u1EA7n xu\u1EA5t PDF.",
            L"Overlay Label",
            MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring defaultName = L"NhanPhongMay_" + SanitizeFilePart(job_.prefix) + L"_" +
                               std::to_wstring(low) + L"-" + std::to_wstring(high) + L".pdf";

    wchar_t filePath[MAX_PATH] = {};
    size_t copyLen = defaultName.size() < (MAX_PATH - 1) ? defaultName.size() : (MAX_PATH - 1);
    std::wmemcpy(filePath, defaultName.c_str(), copyLen);
    filePath[copyLen] = L'\0';

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (previewHwnd_ && IsWindow(previewHwnd_)) ? previewHwnd_ : hwnd_;
    ofn.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"pdf";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameW(&ofn)) {
        return;
    }

    HCURSOR oldCursor = SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    std::wstring err;
    bool ok = ExportLabelsToPdf(filePath, style_, job_, err);
    SetCursor(oldCursor);

    if (!ok) {
        MessageBoxW(ofn.hwndOwner, err.c_str(), L"L\u1ED7i xu\u1EA5t PDF", MB_OK | MB_ICONERROR);
        return;
    }

    ShellExecuteW(nullptr, L"open", filePath, nullptr, nullptr, SW_SHOWNORMAL);
    std::wstring msg = L"\u0110\u00E3 xu\u1EA5t th\u00E0nh c\u00F4ng file PDF:\n" + std::wstring(filePath);
    MessageBoxW(ofn.hwndOwner, msg.c_str(), L"Overlay Label", MB_OK | MB_ICONINFORMATION);
}

void PrintDialog::HandleDirectPrint() {
    ReadJobFromControls();

    HWND owner = (previewHwnd_ && IsWindow(previewHwnd_)) ? previewHwnd_ : hwnd_;
    HCURSOR oldCursor = SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    std::wstring err;
    bool ok = PrintLabelsDirectly(owner, style_, job_, err);
    SetCursor(oldCursor);

    if (!ok && !err.empty()) {
        MessageBoxW(owner, err.c_str(), L"L\u1ED7i in nh\u00E3n", MB_OK | MB_ICONERROR);
    }
}

LRESULT PrintDialog::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateControls();
            SyncControlsFromJob();
            return 0;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            HWND hCtrl = reinterpret_cast<HWND>(lParam);
            SetBkMode(hdc, TRANSPARENT);
            if (GetDlgCtrlID(hCtrl) == IDC_PR_SUMMARY) {
                SetTextColor(hdc, RGB(30, 90, 140));
            }
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }

        case WM_DRAWITEM: {
            auto* dis = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
            if (dis && dis->CtlID == IDC_PR_PREVIEW) {
                RenderPrintPreviewCard(dis->hDC, dis->rcItem, style_, job_);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND: {
            if (syncing_) return 0;
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            switch (id) {
                case IDC_PR_PREFIX:
                case IDC_PR_SEP:
                case IDC_PR_FROM:
                case IDC_PR_TO:
                case IDC_PR_SUB:
                    if (code == EN_CHANGE) {
                        ReadJobFromControls();
                        UpdateSummaryAndPreview();
                    }
                    return 0;

                case IDC_PR_LAYOUT:
                    if (code == CBN_SELCHANGE) {
                        ReadJobFromControls();
                        UpdateSummaryAndPreview();
                    }
                    return 0;

                case IDC_PR_PADZERO:
                case IDC_PR_CUTLINES:
                    ReadJobFromControls();
                    UpdateSummaryAndPreview();
                    return 0;

                case IDC_PR_OPEN_A4_PREVIEW:
                    OpenA4PreviewWindow();
                    return 0;

                case IDC_PR_EXPORT_PDF:
                    HandleExportPdf();
                    return 0;

                case IDC_PR_PRINT:
                    HandleDirectPrint();
                    return 0;

                case IDC_PR_CLOSE:
                    DestroyWindow(hwnd_);
                    return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd_);
            return 0;

        case WM_DESTROY:
            if (previewHwnd_ && IsWindow(previewHwnd_)) {
                DestroyWindow(previewHwnd_);
            }
            previewHwnd_ = nullptr;
            if (hFont_) {
                DeleteObject(hFont_);
                hFont_ = nullptr;
            }
            if (hBoldFont_) {
                DeleteObject(hBoldFont_);
                hBoldFont_ = nullptr;
            }
            hwnd_ = nullptr;
            return 0;
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}
