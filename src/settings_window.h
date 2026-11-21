#pragma once

#include "config.h"

#include <windows.h>
#include <functional>

class SettingsWindow {
public:
    using PreviewCallback = std::function<void(const OverlayConfig&)>;
    using SaveCallback = std::function<void(const OverlayConfig&, bool autoStart)>;
    using CloseCallback = std::function<void()>;
    using PrintCallback = std::function<void()>;

    SettingsWindow() = default;
    ~SettingsWindow();

    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    void Open(HINSTANCE hInstance,
              HICON hAppIcon,
              const OverlayConfig& currentConfig,
              bool currentAutoStart,
              PreviewCallback onPreview,
              SaveCallback onSave,
              CloseCallback onClosed = nullptr,
              PrintCallback onOpenPrint = nullptr);

    void Close();
    void SyncPositionFromExternal(OverlayPosition pos, int marginX, int marginY);
    const OverlayConfig& GetDraftConfig() const { return draftConfig_; }
    HWND GetHandle() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void SyncControlsFromDraft();
    void UpdateValueLabels();
    void UpdateEnabledStates();
    void NotifyPreview(bool immediateSave = true);
    void ScheduleAutoSave();
    void FlushAutoSave();
    bool PickColor(COLORREF& color);
    void PickFont();
    void PickLogo();
    void DrawColorButton(const DRAWITEMSTRUCT* dis, COLORREF color, bool enabled);
    int S(int px) const;

    HWND hwnd_ = nullptr;
    HINSTANCE hInstance_ = nullptr;
    HFONT hFont_ = nullptr;
    HFONT hBoldFont_ = nullptr;
    UINT dpi_ = 96;
    bool classRegistered_ = false;
    bool syncing_ = false;
    bool cancelled_ = false;
    bool dirty_ = false;

    OverlayConfig originalConfig_;
    bool originalAutoStart_ = false;
    OverlayConfig draftConfig_;
    bool draftAutoStart_ = false;
    COLORREF customColors_[16] = {};

    PreviewCallback onPreview_;
    SaveCallback onSave_;
    CloseCallback onClosed_;
    PrintCallback onOpenPrint_;
};
