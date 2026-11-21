#pragma once

#include <windows.h>
#include <string>

enum class OverlayPosition {
    TopRight,
    TopLeft,
    BottomRight,
    BottomLeft
};

enum class FrameStyle {
    Cyber,
    Pill,
    None
};

enum class StylePreset {
    CyberNet = 0,
    VipGold = 1,
    EsportsRed = 2,
    MatrixGreen = 3,
    GlassModern = 4,
    MinimalText = 5
};

constexpr int kPresetCount = 6;

struct OverlayConfig {
    // [Display]
    std::wstring format = L"{username}";
    std::wstring subText = L"ONLINE  \u2022  {time}";
    bool uppercase = true;
    OverlayPosition position = OverlayPosition::TopRight;
    int marginX = 18;
    int marginY = 14;
    bool allMonitors = true;
    bool respectTaskbar = true;

    // [Style]
    FrameStyle frameStyle = FrameStyle::Cyber;
    int scalePercent = 100;
    std::wstring fontName = L"Segoe UI";
    int fontSize = 16;
    bool bold = true;
    COLORREF textColor = RGB(255, 255, 255);
    BYTE opacity = 245;

    COLORREF accentColor = RGB(0, 229, 255);
    bool showStatusDot = false;
    std::wstring logoPath;

    bool shadow = true;
    COLORREF shadowColor = RGB(0, 0, 0);

    bool background = true;
    COLORREF backgroundColor = RGB(11, 18, 32);
    BYTE backgroundOpacity = 205;
    int paddingX = 16;
    int paddingY = 9;
    int cornerRadius = 8;
};

OverlayConfig MakePresetConfig(StylePreset preset, const OverlayConfig& base = OverlayConfig{});
const wchar_t* GetPresetDisplayName(StylePreset preset);

std::wstring GetExecutablePath();
std::wstring GetExecutableDirectory();
std::wstring GetConfigFilePath();
std::wstring GetCurrentUserNameString();
std::wstring GetCurrentComputerNameString();
std::wstring FormatOverlayText(const std::wstring& templateStr, bool uppercase = false);

bool EnsureDefaultConfigExists(const std::wstring& path);
OverlayConfig LoadConfig(const std::wstring& path);
bool SaveConfig(const std::wstring& path, const OverlayConfig& cfg);
bool SaveAllMonitorsSetting(const std::wstring& path, bool allMonitors);
