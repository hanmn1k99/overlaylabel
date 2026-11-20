#include "config.h"

#include <shlobj.h>
#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <iterator>
#include <vector>

namespace {

std::wstring Trim(const std::wstring& s) {
    size_t start = 0;
    while (start < s.size() && std::iswspace(s[start])) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::iswspace(s[end - 1])) {
        --end;
    }
    return s.substr(start, end - start);
}

std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return s;
}

std::wstring ToUpperString(const std::wstring& s) {
    if (s.empty()) return s;
    std::vector<wchar_t> buf(s.begin(), s.end());
    buf.push_back(L'\0');
    CharUpperBuffW(buf.data(), static_cast<DWORD>(s.size()));
    return std::wstring(buf.data(), s.size());
}

std::wstring ReadIniString(const std::wstring& path,
                           const wchar_t* section,
                           const wchar_t* key,
                           const wchar_t* defaultValue) {
    wchar_t buffer[512] = {};
    GetPrivateProfileStringW(section, key, defaultValue, buffer, 512, path.c_str());
    return Trim(buffer);
}

int ClampInt(int value, int minVal, int maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

COLORREF ParseHexColor(const std::wstring& hexStr, COLORREF fallback) {
    std::wstring s = Trim(hexStr);
    if (!s.empty() && s[0] == L'#') {
        s.erase(s.begin());
    }
    if (s.size() != 6) {
        return fallback;
    }
    try {
        unsigned long val = std::stoul(s, nullptr, 16);
        BYTE r = static_cast<BYTE>((val >> 16) & 0xFF);
        BYTE g = static_cast<BYTE>((val >> 8) & 0xFF);
        BYTE b = static_cast<BYTE>(val & 0xFF);
        return RGB(r, g, b);
    } catch (...) {
        return fallback;
    }
}

OverlayPosition ParsePosition(const std::wstring& posStr) {
    std::wstring lower = ToLower(Trim(posStr));
    if (lower == L"top-left" || lower == L"topleft") return OverlayPosition::TopLeft;
    if (lower == L"bottom-right" || lower == L"bottomright") return OverlayPosition::BottomRight;
    if (lower == L"bottom-left" || lower == L"bottomleft") return OverlayPosition::BottomLeft;
    return OverlayPosition::TopRight;
}

const wchar_t* PositionToString(OverlayPosition pos) {
    switch (pos) {
        case OverlayPosition::TopLeft: return L"top-left";
        case OverlayPosition::BottomRight: return L"bottom-right";
        case OverlayPosition::BottomLeft: return L"bottom-left";
        case OverlayPosition::TopRight:
        default:
            return L"top-right";
    }
}

FrameStyle ParseFrameStyle(const std::wstring& str) {
    std::wstring lower = ToLower(Trim(str));
    if (lower == L"pill" || lower == L"rounded") return FrameStyle::Pill;
    if (lower == L"none" || lower == L"text") return FrameStyle::None;
    return FrameStyle::Cyber;
}

const wchar_t* FrameStyleToString(FrameStyle style) {
    switch (style) {
        case FrameStyle::Pill: return L"pill";
        case FrameStyle::None: return L"none";
        case FrameStyle::Cyber:
        default:
            return L"cyber";
    }
}

std::wstring FormatHexColor(COLORREF c) {
    wchar_t buf[16] = {};
    std::swprintf(buf, 16, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
    return buf;
}

void ReplaceAll(std::wstring& text, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::wstring::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::wstring GetCurrentTimeString() {
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    wchar_t buf[16] = {};
    std::swprintf(buf, 16, L"%02u:%02u", st.wHour, st.wMinute);
    return buf;
}

std::wstring GetCurrentDateString() {
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    wchar_t buf[24] = {};
    std::swprintf(buf, 24, L"%02u/%02u/%04u", st.wDay, st.wMonth, st.wYear);
    return buf;
}

} // namespace

OverlayConfig MakePresetConfig(StylePreset preset, const OverlayConfig& base) {
    OverlayConfig cfg = base;
    switch (preset) {
        case StylePreset::CyberNet:
            cfg.frameStyle = FrameStyle::Cyber;
            cfg.subText = L"ONLINE  \u2022  {time}";
            cfg.uppercase = true;
            cfg.fontName = L"Segoe UI";
            cfg.fontSize = 16;
            cfg.bold = true;
            cfg.textColor = RGB(255, 255, 255);
            cfg.opacity = 245;
            cfg.accentColor = RGB(0, 229, 255);
            cfg.showStatusDot = true;
            cfg.shadow = true;
            cfg.shadowColor = RGB(0, 0, 0);
            cfg.background = true;
            cfg.backgroundColor = RGB(11, 18, 32);
            cfg.backgroundOpacity = 205;
            cfg.paddingX = 16;
            cfg.paddingY = 9;
            cfg.cornerRadius = 8;
            break;

        case StylePreset::VipGold:
            cfg.frameStyle = FrameStyle::Cyber;
            cfg.subText = L"VIP ZONE  \u2022  {time}";
            cfg.uppercase = true;
            cfg.fontName = L"Segoe UI";
            cfg.fontSize = 16;
            cfg.bold = true;
            cfg.textColor = RGB(255, 247, 224);
            cfg.opacity = 245;
            cfg.accentColor = RGB(255, 184, 0);
            cfg.showStatusDot = true;
            cfg.shadow = true;
            cfg.shadowColor = RGB(0, 0, 0);
            cfg.background = true;
            cfg.backgroundColor = RGB(20, 17, 11);
            cfg.backgroundOpacity = 210;
            cfg.paddingX = 16;
            cfg.paddingY = 9;
            cfg.cornerRadius = 8;
            break;

        case StylePreset::EsportsRed:
            cfg.frameStyle = FrameStyle::Cyber;
            cfg.subText = L"GAMING  \u2022  {time}";
            cfg.uppercase = true;
            cfg.fontName = L"Segoe UI";
            cfg.fontSize = 16;
            cfg.bold = true;
            cfg.textColor = RGB(255, 255, 255);
            cfg.opacity = 245;
            cfg.accentColor = RGB(255, 46, 85);
            cfg.showStatusDot = true;
            cfg.shadow = true;
            cfg.shadowColor = RGB(0, 0, 0);
            cfg.background = true;
            cfg.backgroundColor = RGB(22, 11, 16);
            cfg.backgroundOpacity = 210;
            cfg.paddingX = 16;
            cfg.paddingY = 9;
            cfg.cornerRadius = 8;
            break;

        case StylePreset::MatrixGreen:
            cfg.frameStyle = FrameStyle::Cyber;
            cfg.subText = L"NET STATION  \u2022  {time}";
            cfg.uppercase = true;
            cfg.fontName = L"Consolas";
            cfg.fontSize = 16;
            cfg.bold = true;
            cfg.textColor = RGB(226, 255, 236);
            cfg.opacity = 245;
            cfg.accentColor = RGB(34, 211, 102);
            cfg.showStatusDot = true;
            cfg.shadow = true;
            cfg.shadowColor = RGB(0, 0, 0);
            cfg.background = true;
            cfg.backgroundColor = RGB(8, 22, 16);
            cfg.backgroundOpacity = 210;
            cfg.paddingX = 16;
            cfg.paddingY = 9;
            cfg.cornerRadius = 8;
            break;

        case StylePreset::GlassModern:
            cfg.frameStyle = FrameStyle::Pill;
            cfg.subText = L"";
            cfg.uppercase = false;
            cfg.fontName = L"Segoe UI";
            cfg.fontSize = 16;
            cfg.bold = true;
            cfg.textColor = RGB(255, 255, 255);
            cfg.opacity = 235;
            cfg.accentColor = RGB(56, 189, 248);
            cfg.showStatusDot = true;
            cfg.shadow = true;
            cfg.shadowColor = RGB(0, 0, 0);
            cfg.background = true;
            cfg.backgroundColor = RGB(18, 24, 38);
            cfg.backgroundOpacity = 165;
            cfg.paddingX = 15;
            cfg.paddingY = 8;
            cfg.cornerRadius = 12;
            break;

        case StylePreset::MinimalText:
            cfg.frameStyle = FrameStyle::None;
            cfg.subText = L"";
            cfg.uppercase = false;
            cfg.fontName = L"Segoe UI";
            cfg.fontSize = 18;
            cfg.bold = true;
            cfg.textColor = RGB(255, 255, 255);
            cfg.opacity = 225;
            cfg.accentColor = RGB(56, 189, 248);
            cfg.showStatusDot = false;
            cfg.shadow = true;
            cfg.shadowColor = RGB(0, 0, 0);
            cfg.background = false;
            cfg.backgroundColor = RGB(20, 20, 20);
            cfg.backgroundOpacity = 0;
            cfg.paddingX = 8;
            cfg.paddingY = 4;
            cfg.cornerRadius = 8;
            break;
    }
    return cfg;
}

const wchar_t* GetPresetDisplayName(StylePreset preset) {
    switch (preset) {
        case StylePreset::CyberNet: return L"Cyber Neon (Xanh)";
        case StylePreset::VipGold: return L"VIP Gold (Ho\u00E0ng kim)";
        case StylePreset::EsportsRed: return L"Esports Red (\u0110\u1ECF ROG)";
        case StylePreset::MatrixGreen: return L"Matrix Net (Xanh l\u00E1)";
        case StylePreset::GlassModern: return L"K\u00EDnh m\u1EDD (Bo tr\u00F2n)";
        case StylePreset::MinimalText: return L"T\u1ED1i gi\u1EA3n (Ch\u1EC9 ch\u1EEF)";
    }
    return L"Cyber Neon (Xanh)";
}

std::wstring GetExecutablePath() {
    std::vector<wchar_t> buf(MAX_PATH);
    while (true) {
        DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (len == 0) {
            return L"";
        }
        if (len < buf.size() - 1) {
            return std::wstring(buf.data(), len);
        }
        buf.resize(buf.size() * 2);
    }
}

std::wstring GetExecutableDirectory() {
    std::wstring exePath = GetExecutablePath();
    size_t slash = exePath.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    return exePath.substr(0, slash);
}

std::wstring GetConfigFilePath() {
    // Portable mode if portable.txt exists next to the executable
    std::wstring exeDir = GetExecutableDirectory();
    std::wstring portableFlag = exeDir + L"\\portable.txt";
    if (GetFileAttributesW(portableFlag.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return exeDir + L"\\config.ini";
    }

    // Standard per-user AppData path (works cleanly when installed in Program Files)
    wchar_t appData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appData))) {
        std::wstring dir = std::wstring(appData) + L"\\OverlayLabel";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\config.ini";
    }
    return exeDir + L"\\config.ini";
}

std::wstring GetCurrentUserNameString() {
    wchar_t buffer[256] = {};
    DWORD size = static_cast<DWORD>(std::size(buffer));
    if (GetUserNameW(buffer, &size) && size > 1) {
        return std::wstring(buffer);
    }
    return L"Unknown";
}

std::wstring GetCurrentComputerNameString() {
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = static_cast<DWORD>(std::size(buffer));
    if (GetComputerNameW(buffer, &size) && size > 0) {
        return std::wstring(buffer);
    }
    return L"PC";
}

std::wstring FormatOverlayText(const std::wstring& templateStr, bool uppercase) {
    if (templateStr.empty()) {
        return L"";
    }
    std::wstring result = templateStr;
    ReplaceAll(result, L"{username}", GetCurrentUserNameString());
    ReplaceAll(result, L"{computer}", GetCurrentComputerNameString());
    ReplaceAll(result, L"{time}", GetCurrentTimeString());
    ReplaceAll(result, L"{date}", GetCurrentDateString());
    if (uppercase) {
        result = ToUpperString(result);
    }
    return result;
}

bool EnsureDefaultConfigExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    const wchar_t* defaultIni =
        L"; Cau hinh Overlay Label\r\n"
        L"; Duoc quan ly tu dong qua cua so 'Cai dat...'.\r\n"
        L"\r\n"
        L"[Display]\r\n"
        L"; Ho tro bien: {username}, {computer}, {time}, {date}\r\n"
        L"format={username}\r\n"
        L"sub_text=ONLINE  \u2022  {time}\r\n"
        L"uppercase=1\r\n"
        L"; Vi tri: top-right, top-left, bottom-right, bottom-left\r\n"
        L"position=top-right\r\n"
        L"margin_x=18\r\n"
        L"margin_y=14\r\n"
        L"all_monitors=1\r\n"
        L"respect_taskbar=1\r\n"
        L"\r\n"
        L"[Style]\r\n"
        L"; Kieu khung: cyber, pill, none\r\n"
        L"frame_style=cyber\r\n"
        L"scale=100\r\n"
        L"font_name=Segoe UI\r\n"
        L"font_size=16\r\n"
        L"bold=1\r\n"
        L"text_color=#FFFFFF\r\n"
        L"opacity=245\r\n"
        L"accent_color=#00E5FF\r\n"
        L"show_status_dot=1\r\n"
        L"shadow=1\r\n"
        L"shadow_color=#000000\r\n"
        L"background=1\r\n"
        L"background_color=#0B1220\r\n"
        L"background_opacity=205\r\n"
        L"padding_x=16\r\n"
        L"padding_y=9\r\n"
        L"corner_radius=8\r\n";

    const WORD bom = 0xFEFF;
    DWORD written = 0;
    WriteFile(hFile, &bom, sizeof(bom), &written, nullptr);
    WriteFile(hFile, defaultIni, static_cast<DWORD>(wcslen(defaultIni) * sizeof(wchar_t)), &written, nullptr);
    CloseHandle(hFile);
    return true;
}

OverlayConfig LoadConfig(const std::wstring& path) {
    EnsureDefaultConfigExists(path);

    OverlayConfig cfg;
    cfg.format = ReadIniString(path, L"Display", L"format", L"{username}");
    if (cfg.format.empty()) {
        cfg.format = L"{username}";
    }
    cfg.subText = ReadIniString(path, L"Display", L"sub_text", L"ONLINE  \u2022  {time}");
    cfg.uppercase = GetPrivateProfileIntW(L"Display", L"uppercase", 1, path.c_str()) != 0;
    cfg.position = ParsePosition(ReadIniString(path, L"Display", L"position", L"top-right"));
    cfg.marginX = ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Display", L"margin_x", 18, path.c_str())), 0, 500);
    cfg.marginY = ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Display", L"margin_y", 14, path.c_str())), 0, 500);
    cfg.allMonitors = GetPrivateProfileIntW(L"Display", L"all_monitors", 1, path.c_str()) != 0;
    cfg.respectTaskbar = GetPrivateProfileIntW(L"Display", L"respect_taskbar", 1, path.c_str()) != 0;

    cfg.frameStyle = ParseFrameStyle(ReadIniString(path, L"Style", L"frame_style", L"cyber"));
    cfg.scalePercent = ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Style", L"scale", 100, path.c_str())), 50, 250);
    cfg.fontName = ReadIniString(path, L"Style", L"font_name", L"Segoe UI");
    if (cfg.fontName.empty()) {
        cfg.fontName = L"Segoe UI";
    }
    cfg.fontSize = ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Style", L"font_size", 16, path.c_str())), 8, 120);
    cfg.bold = GetPrivateProfileIntW(L"Style", L"bold", 1, path.c_str()) != 0;
    cfg.textColor = ParseHexColor(ReadIniString(path, L"Style", L"text_color", L"#FFFFFF"), RGB(255, 255, 255));
    cfg.opacity = static_cast<BYTE>(ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Style", L"opacity", 245, path.c_str())), 10, 255));

    cfg.accentColor = ParseHexColor(ReadIniString(path, L"Style", L"accent_color", L"#00E5FF"), RGB(0, 229, 255));
    cfg.showStatusDot = GetPrivateProfileIntW(L"Style", L"show_status_dot", 1, path.c_str()) != 0;

    cfg.shadow = GetPrivateProfileIntW(L"Style", L"shadow", 1, path.c_str()) != 0;
    cfg.shadowColor = ParseHexColor(ReadIniString(path, L"Style", L"shadow_color", L"#000000"), RGB(0, 0, 0));

    cfg.background = GetPrivateProfileIntW(L"Style", L"background", 1, path.c_str()) != 0;
    cfg.backgroundColor = ParseHexColor(ReadIniString(path, L"Style", L"background_color", L"#0B1220"), RGB(11, 18, 32));
    cfg.backgroundOpacity = static_cast<BYTE>(ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Style", L"background_opacity", 205, path.c_str())), 0, 255));
    cfg.paddingX = ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Style", L"padding_x", 16, path.c_str())), 0, 100);
    cfg.paddingY = ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Style", L"padding_y", 9, path.c_str())), 0, 100);
    cfg.cornerRadius = ClampInt(static_cast<int>(GetPrivateProfileIntW(L"Style", L"corner_radius", 8, path.c_str())), 0, 64);

    return cfg;
}

bool SaveConfig(const std::wstring& path, const OverlayConfig& cfg) {
    EnsureDefaultConfigExists(path);

    auto writeInt = [&](const wchar_t* sec, const wchar_t* key, int val) {
        std::wstring s = std::to_wstring(val);
        WritePrivateProfileStringW(sec, key, s.c_str(), path.c_str());
    };

    WritePrivateProfileStringW(L"Display", L"format", cfg.format.c_str(), path.c_str());
    WritePrivateProfileStringW(L"Display", L"sub_text", cfg.subText.c_str(), path.c_str());
    writeInt(L"Display", L"uppercase", cfg.uppercase ? 1 : 0);
    WritePrivateProfileStringW(L"Display", L"position", PositionToString(cfg.position), path.c_str());
    writeInt(L"Display", L"margin_x", cfg.marginX);
    writeInt(L"Display", L"margin_y", cfg.marginY);
    writeInt(L"Display", L"all_monitors", cfg.allMonitors ? 1 : 0);
    writeInt(L"Display", L"respect_taskbar", cfg.respectTaskbar ? 1 : 0);

    WritePrivateProfileStringW(L"Style", L"frame_style", FrameStyleToString(cfg.frameStyle), path.c_str());
    writeInt(L"Style", L"scale", cfg.scalePercent);
    WritePrivateProfileStringW(L"Style", L"font_name", cfg.fontName.c_str(), path.c_str());
    writeInt(L"Style", L"font_size", cfg.fontSize);
    writeInt(L"Style", L"bold", cfg.bold ? 1 : 0);
    WritePrivateProfileStringW(L"Style", L"text_color", FormatHexColor(cfg.textColor).c_str(), path.c_str());
    writeInt(L"Style", L"opacity", static_cast<int>(cfg.opacity));

    WritePrivateProfileStringW(L"Style", L"accent_color", FormatHexColor(cfg.accentColor).c_str(), path.c_str());
    writeInt(L"Style", L"show_status_dot", cfg.showStatusDot ? 1 : 0);

    writeInt(L"Style", L"shadow", cfg.shadow ? 1 : 0);
    WritePrivateProfileStringW(L"Style", L"shadow_color", FormatHexColor(cfg.shadowColor).c_str(), path.c_str());

    writeInt(L"Style", L"background", cfg.background ? 1 : 0);
    WritePrivateProfileStringW(L"Style", L"background_color", FormatHexColor(cfg.backgroundColor).c_str(), path.c_str());
    writeInt(L"Style", L"background_opacity", static_cast<int>(cfg.backgroundOpacity));
    writeInt(L"Style", L"padding_x", cfg.paddingX);
    writeInt(L"Style", L"padding_y", cfg.paddingY);
    writeInt(L"Style", L"corner_radius", cfg.cornerRadius);

    return true;
}

bool SaveAllMonitorsSetting(const std::wstring& path, bool allMonitors) {
    EnsureDefaultConfigExists(path);
    return WritePrivateProfileStringW(L"Display", L"all_monitors", allMonitors ? L"1" : L"0", path.c_str()) != FALSE;
}
