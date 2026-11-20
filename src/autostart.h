#pragma once

#include <string>

bool IsAutoStartEnabled(const std::wstring& appName);
bool SetAutoStartEnabled(const std::wstring& appName, bool enable);
