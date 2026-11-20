#include "autostart.h"
#include "config.h"

#include <windows.h>

namespace {
constexpr const wchar_t* kRunRegPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
}

bool IsAutoStartEnabled(const std::wstring& appName) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunRegPath, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD type = 0;
    DWORD dataSize = 0;
    LONG status = RegQueryValueExW(hKey, appName.c_str(), nullptr, &type, nullptr, &dataSize);
    RegCloseKey(hKey);

    return status == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ) && dataSize > sizeof(wchar_t);
}

bool SetAutoStartEnabled(const std::wstring& appName, bool enable) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunRegPath, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    LONG status = ERROR_SUCCESS;
    if (enable) {
        std::wstring exePath = GetExecutablePath();
        if (exePath.empty()) {
            RegCloseKey(hKey);
            return false;
        }
        std::wstring quotedPath = L"\"" + exePath + L"\"";
        DWORD byteSize = static_cast<DWORD>((quotedPath.size() + 1) * sizeof(wchar_t));
        status = RegSetValueExW(
            hKey,
            appName.c_str(),
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(quotedPath.c_str()),
            byteSize);
    } else {
        status = RegDeleteValueW(hKey, appName.c_str());
        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }
    }

    RegCloseKey(hKey);
    return status == ERROR_SUCCESS;
}
