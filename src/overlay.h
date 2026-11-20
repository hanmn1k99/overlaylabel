#pragma once

#include "config.h"

#include <windows.h>
#include <vector>

class OverlayManager {
public:
    OverlayManager() = default;
    ~OverlayManager();

    OverlayManager(const OverlayManager&) = delete;
    OverlayManager& operator=(const OverlayManager&) = delete;

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();

    void Rebuild(const OverlayConfig& config);
    void UpdateContent(const OverlayConfig& config);
    void SetVisible(bool visible);
    bool IsVisible() const { return visible_; }
    void EnsureTopMost();
    HICON CreateAppTrayIcon() const;

private:
    struct MonitorTarget {
        HMONITOR hMonitor = nullptr;
        RECT workArea = {};
        RECT monitorArea = {};
        bool isPrimary = false;
    };

    struct OverlayInstance {
        HWND hwnd = nullptr;
        MonitorTarget target;
    };

    static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData);

    void DestroyAllWindows();
    HWND CreateOverlayWindow();
    void RenderToWindow(HWND hwnd,
                        const MonitorTarget& target,
                        const OverlayConfig& config,
                        const std::wstring& mainText,
                        const std::wstring& subText);
    static UINT GetMonitorDpi(HMONITOR hMonitor);

    HINSTANCE hInstance_ = nullptr;
    ULONG_PTR gdiplusToken_ = 0;
    bool gdiplusStarted_ = false;
    bool classRegistered_ = false;
    bool visible_ = true;
    std::vector<OverlayInstance> windows_;
};
