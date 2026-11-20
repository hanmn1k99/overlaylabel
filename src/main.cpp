#include "autostart.h"
#include "config.h"
#include "overlay.h"
#include "settings_window.h"

#include <windows.h>
#include <shellapi.h>
#include <cwchar>
#include <string>

namespace {

constexpr const wchar_t* kAppName = L"OverlayLabel";
constexpr const wchar_t* kMutexName = L"Local\\OverlayLabel_SingleInstance_Mutex";
constexpr const wchar_t* kControllerClassName = L"OverlayLabelControllerClass";

constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT_PTR IDT_MAINTENANCE = 1;
constexpr UINT_PTR IDT_REBUILD_DEBOUNCE = 2;

constexpr UINT ID_APP_ICON = 101;
constexpr UINT ID_TRAY_HEADER = 1000;
constexpr UINT ID_TRAY_SETTINGS = 1001;
constexpr UINT ID_TRAY_TOGGLE_VISIBLE = 1002;
constexpr UINT ID_TRAY_ALL_MONITORS = 1003;
constexpr UINT ID_TRAY_AUTOSTART = 1004;
constexpr UINT ID_TRAY_EXIT = 1006;
constexpr UINT ID_TRAY_PRESET_BASE = 1100;

HINSTANCE g_hInstance = nullptr;
OverlayManager g_overlay;
SettingsWindow g_settingsWindow;
OverlayConfig g_config;
std::wstring g_configPath;
std::wstring g_lastRenderedClockKey;
NOTIFYICONDATAW g_nid = {};
HICON g_trayIcon = nullptr;
HICON g_appIconBig = nullptr;
UINT g_wmTaskbarCreated = 0;
UINT g_wmShowSettings = 0;

void EnableDpiAwareness() {
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto pSetDpiContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            reinterpret_cast<void*>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext")));
        if (pSetDpiContext) {
            if (pSetDpiContext(reinterpret_cast<HANDLE>(-4))) {
                return;
            }
        }
    }
    SetProcessDPIAware();
}

void UpdateClockKey() {
    g_lastRenderedClockKey = FormatOverlayText(g_config.format, g_config.uppercase) + L"\n" +
                             FormatOverlayText(g_config.subText, g_config.uppercase);
}

void ReloadConfigAndRebuild() {
    g_config = LoadConfig(g_configPath);
    UpdateClockKey();
    g_overlay.Rebuild(g_config);
}

void ApplyPresetAndSave(StylePreset preset) {
    g_config = MakePresetConfig(preset, g_config);
    SaveConfig(g_configPath, g_config);
    UpdateClockKey();
    g_overlay.Rebuild(g_config);
}

void OpenSettingsDialog() {
    HICON hIcon = g_appIconBig ? g_appIconBig : g_trayIcon;
    g_settingsWindow.Open(
        g_hInstance,
        hIcon,
        g_config,
        IsAutoStartEnabled(kAppName),
        [](const OverlayConfig& previewCfg) {
            g_overlay.Rebuild(previewCfg);
        },
        [](const OverlayConfig& newCfg, bool autoStart) {
            g_config = newCfg;
            SaveConfig(g_configPath, g_config);
            SetAutoStartEnabled(kAppName, autoStart);
            UpdateClockKey();
            g_overlay.Rebuild(g_config);
        });
}

void AddTrayIcon(HWND hwnd) {
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_trayIcon;

    std::wstring tip = L"Overlay Label (" + GetCurrentUserNameString() + L")";
    constexpr size_t maxTipLen = (sizeof(g_nid.szTip) / sizeof(g_nid.szTip[0])) - 1;
    size_t copyLen = tip.size() < maxTipLen ? tip.size() : maxTipLen;
    std::wmemcpy(g_nid.szTip, tip.c_str(), copyLen);
    g_nid.szTip[copyLen] = L'\0';

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    if (g_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_nid.hWnd = nullptr;
    }
}

void ShowTrayMenu(HWND hwnd) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    std::wstring header = L"Overlay Label \u2014 " + GetCurrentUserNameString();
    AppendMenuW(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED, ID_TRAY_HEADER, header.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"C\u00E0i \u0111\u1EB7t...");
    SetMenuDefaultItem(hMenu, ID_TRAY_SETTINGS, FALSE);

    HMENU hPresetMenu = CreatePopupMenu();
    if (hPresetMenu) {
        for (int i = 0; i < kPresetCount; ++i) {
            auto preset = static_cast<StylePreset>(i);
            AppendMenuW(hPresetMenu, MF_STRING, ID_TRAY_PRESET_BASE + static_cast<UINT>(i), GetPresetDisplayName(preset));
        }
        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hPresetMenu), L"M\u1EABu giao di\u1EC7n");
    }

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    UINT visibleFlags = MF_STRING | (g_overlay.IsVisible() ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(hMenu, visibleFlags, ID_TRAY_TOGGLE_VISIBLE, L"Hi\u1EC3n th\u1ECB Overlay");

    UINT allMonFlags = MF_STRING | (g_config.allMonitors ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(hMenu, allMonFlags, ID_TRAY_ALL_MONITORS, L"Hi\u1EC3n th\u1ECB tr\u00EAn t\u1EA5t c\u1EA3 m\u00E0n h\u00ECnh");

    UINT autoStartFlags = MF_STRING | (IsAutoStartEnabled(kAppName) ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(hMenu, autoStartFlags, ID_TRAY_AUTOSTART, L"Kh\u1EDFi \u0111\u1ED9ng c\u00F9ng Windows");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Tho\u00E1t");

    POINT pt = {};
    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

LRESULT CALLBACK ControllerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_wmTaskbarCreated != 0 && msg == g_wmTaskbarCreated) {
        AddTrayIcon(hwnd);
        return 0;
    }
    if (g_wmShowSettings != 0 && msg == g_wmShowSettings) {
        OpenSettingsDialog();
        return 0;
    }

    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                ShowTrayMenu(hwnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                OpenSettingsDialog();
            }
            return 0;

        case WM_COMMAND: {
            UINT cmdId = LOWORD(wParam);
            if (cmdId >= ID_TRAY_PRESET_BASE && cmdId < ID_TRAY_PRESET_BASE + static_cast<UINT>(kPresetCount)) {
                ApplyPresetAndSave(static_cast<StylePreset>(cmdId - ID_TRAY_PRESET_BASE));
                return 0;
            }
            switch (cmdId) {
                case ID_TRAY_SETTINGS:
                    OpenSettingsDialog();
                    return 0;

                case ID_TRAY_TOGGLE_VISIBLE:
                    g_overlay.SetVisible(!g_overlay.IsVisible());
                    return 0;

                case ID_TRAY_ALL_MONITORS:
                    g_config.allMonitors = !g_config.allMonitors;
                    SaveAllMonitorsSetting(g_configPath, g_config.allMonitors);
                    g_overlay.Rebuild(g_config);
                    return 0;

                case ID_TRAY_AUTOSTART: {
                    bool currentlyEnabled = IsAutoStartEnabled(kAppName);
                    SetAutoStartEnabled(kAppName, !currentlyEnabled);
                    return 0;
                }

                case ID_TRAY_EXIT:
                    g_settingsWindow.Close();
                    DestroyWindow(hwnd);
                    return 0;
            }
            break;
        }

        case WM_DISPLAYCHANGE:
            SetTimer(hwnd, IDT_REBUILD_DEBOUNCE, 300, nullptr);
            return 0;

        case WM_SETTINGCHANGE:
            if (wParam == SPI_SETWORKAREA) {
                SetTimer(hwnd, IDT_REBUILD_DEBOUNCE, 300, nullptr);
            }
            return 0;

        case WM_TIMER:
            if (wParam == IDT_MAINTENANCE) {
                g_overlay.EnsureTopMost();

                if (!g_settingsWindow.GetHandle()) {
                    std::wstring nowKey = FormatOverlayText(g_config.format, g_config.uppercase) + L"\n" +
                                          FormatOverlayText(g_config.subText, g_config.uppercase);
                    if (nowKey != g_lastRenderedClockKey) {
                        g_lastRenderedClockKey = nowKey;
                        g_overlay.UpdateContent(g_config);
                    }
                }
                return 0;
            }
            if (wParam == IDT_REBUILD_DEBOUNCE) {
                KillTimer(hwnd, IDT_REBUILD_DEBOUNCE);
                g_overlay.Rebuild(g_config);
                return 0;
            }
            break;

        case WM_DESTROY:
            KillTimer(hwnd, IDT_MAINTENANCE);
            KillTimer(hwnd, IDT_REBUILD_DEBOUNCE);
            g_settingsWindow.Close();
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    g_hInstance = hInstance;
    EnableDpiAwareness();

    g_wmShowSettings = RegisterWindowMessageW(L"OverlayLabel_ShowSettings");

    HANDLE hMutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        HWND hExisting = FindWindowW(kControllerClassName, L"OverlayLabelController");
        if (hExisting && g_wmShowSettings != 0) {
            PostMessageW(hExisting, g_wmShowSettings, 0, 0);
        }
        return 0;
    }

    if (!g_overlay.Initialize(hInstance)) {
        CloseHandle(hMutex);
        return 1;
    }

    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ControllerWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kControllerClassName;
    if (!RegisterClassExW(&wc)) {
        g_overlay.Shutdown();
        CloseHandle(hMutex);
        return 1;
    }

    HWND hController = CreateWindowExW(
        0,
        kControllerClassName,
        L"OverlayLabelController",
        WS_OVERLAPPED,
        0, 0, 0, 0,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hController) {
        g_overlay.Shutdown();
        CloseHandle(hMutex);
        return 1;
    }

    g_trayIcon = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(ID_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));
    if (!g_trayIcon) {
        g_trayIcon = g_overlay.CreateAppTrayIcon();
    }

    g_appIconBig = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(ID_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_DEFAULTCOLOR));

    AddTrayIcon(hController);

    g_configPath = GetConfigFilePath();
    ReloadConfigAndRebuild();

    SetTimer(hController, IDT_MAINTENANCE, 2000, nullptr);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        HWND hSettings = g_settingsWindow.GetHandle();
        if (hSettings && IsDialogMessageW(hSettings, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_trayIcon) {
        DestroyIcon(g_trayIcon);
        g_trayIcon = nullptr;
    }
    if (g_appIconBig) {
        DestroyIcon(g_appIconBig);
        g_appIconBig = nullptr;
    }
    g_overlay.Shutdown();
    UnregisterClassW(kControllerClassName, hInstance);
    CloseHandle(hMutex);

    return static_cast<int>(msg.wParam);
}
