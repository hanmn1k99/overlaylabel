#pragma once

#include "config.h"
#include "label_printer.h"

#include <windows.h>

class PrintDialog {
public:
    PrintDialog() = default;
    ~PrintDialog();

    PrintDialog(const PrintDialog&) = delete;
    PrintDialog& operator=(const PrintDialog&) = delete;

    void Open(HINSTANCE hInstance, HICON hAppIcon, const OverlayConfig& currentStyle);
    void UpdateStyle(const OverlayConfig& currentStyle);
    void Close();
    HWND GetHandle() const { return hwnd_; }
    HWND GetPreviewHandle() const { return previewHwnd_; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandlePreviewMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void SyncControlsFromJob();
    void ReadJobFromControls();
    void UpdateSummaryAndPreview();
    void OpenA4PreviewWindow();
    void LayoutA4PreviewControls();
    void UpdateA4PreviewNav();
    int GetTotalPages() const;
    void HandleExportPdf();
    void HandleDirectPrint();
    int S(int px) const;

    HWND hwnd_ = nullptr;
    HWND previewHwnd_ = nullptr;
    HINSTANCE hInstance_ = nullptr;
    HICON hAppIcon_ = nullptr;
    HFONT hFont_ = nullptr;
    HFONT hBoldFont_ = nullptr;
    UINT dpi_ = 96;
    bool classRegistered_ = false;
    bool previewClassRegistered_ = false;
    bool syncing_ = false;
    int previewPage_ = 0;

    OverlayConfig style_;
    PrintJobConfig job_;
};
