#pragma once

#include "config.h"

#include <windows.h>
#include <functional>
#include <vector>

class OverlayManager {
public:
    using DragCallback = std::function<void(OverlayPosition pos, int marginX, int marginY, bool finished)>;

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
    void SetDragEnabled(bool enabled, DragCallback onDragged = nullptr);
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
    LRESULT HandleOverlayMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void DestroyAllWindows();
    HWND CreateOverlayWindow();
    void ApplyDragStyleToWindow(HWND hwnd);
    const OverlayInstance* FindInstance(HWND hwnd) const;
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
    bool dragEnabled_ = false;
    DragCallback onDragged_;
    OverlayConfig lastConfig_;

    HWND draggingHwnd_ = nullptr;
    POINT dragStartCursor_ = {};
    RECT dragStartRect_ = {};
    OverlayPosition lastDragPos_ = OverlayPosition::TopRight;
    int lastDragMarginX_ = 18;
    int lastDragMarginY_ = 14;

    std::vector<OverlayInstance> windows_;
};
