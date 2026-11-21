#pragma once

#include "config.h"

#include <windows.h>
#include <string>

struct PrintJobConfig {
    std::wstring prefix = L"MAY";
    std::wstring separator = L" ";
    int startNum = 1;
    int endNum = 20;
    bool padZero = true;
    std::wstring subLabel = L"minhhan.net";
    int layoutIndex = 0; // 0: 2x4 (8/page), 1: 1x3 (3/page), 2: 2x5 (10/page), 3: 3x6 (18/page)
    bool showCutLines = true;
};

void GetPrintLayoutGrid(int layoutIndex, int& outCols, int& outRows);
std::wstring FormatPrintLabelNumber(const PrintJobConfig& job, int num, bool uppercase);

void RenderPrintPreviewCard(HDC hdc, const RECT& rc, const OverlayConfig& style, const PrintJobConfig& job);
void RenderPrintA4PagePreview(HDC hdc, const RECT& rc, const OverlayConfig& style, const PrintJobConfig& job, int pageIndex);

bool ExportLabelsToPdf(const std::wstring& pdfPath,
                       const OverlayConfig& style,
                       const PrintJobConfig& job,
                       std::wstring& outError);

bool PrintLabelsDirectly(HWND hwndOwner,
                         const OverlayConfig& style,
                         const PrintJobConfig& job,
                         std::wstring& outError);
