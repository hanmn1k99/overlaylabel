#pragma once

#include "config.h"

#include <windows.h>
#include <functional>

class SettingsWindow {
public:
    using PreviewCallback = std::function<void(const OverlayConfig&)>;
    using SaveCallback = std::function<void(const OverlayConfig&, bool autoStart)>;

    SettingsWindow() = default;
    ~SettingsWindow();

    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    void Open(HINSTANCE hInstance,
              HICON hAppIcon,
              const OverlayConfig& currentConfig,
              bool currentAutoStart,
              PreviewCallback onPreview,
              SaveCallback onSave);

    void Close();
    HWND GetHandle() const { return hwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void SyncControlsFromDraft();
    void UpdateValueLabels();
    void UpdateEnabledStates();
    void NotifyPreview();
    bool PickColor(COLORREF& color);
    void PickFont();
    void DrawColorButton(const DRAWITEMSTRUCT* dis, COLORREF color, bool enabled);
    int S(int px) const;

    HWND hwnd_ = nullptr;
    HINSTANCE hInstance_ = nullptr;
    HFONT hFont_ = nullptr;
    HFONT hBoldFont_ = nullptr;
    UINT dpi_ = 96;
    bool classRegistered_ = false;
    bool syncing_ = false;
    bool saved_ = false;

    OverlayConfig originalConfig_;
    OverlayConfig draftConfig_;
    bool draftAutoStart_ = false;
    COLORREF customColors_[16] = {};

    PreviewCallback onPreview_;
    SaveCallback onSave_;
};
