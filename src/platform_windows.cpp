#include "platform.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>

namespace Platform {

ScreenBounds GetVirtualScreenBounds() {
    ScreenBounds bounds;
    bounds.x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    bounds.y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    bounds.width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    bounds.height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (bounds.width == 0 || bounds.height == 0) {
        bounds.x = 0;
        bounds.y = 0;
        bounds.width = GetSystemMetrics(SM_CXSCREEN);
        bounds.height = GetSystemMetrics(SM_CYSCREEN);
    }
    return bounds;
}

bool IsStartupEnabled() {
    HKEY hKey;
    LONG res = RegOpenKeyExW(
        HKEY_CURRENT_USER, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_READ, &hKey
    );
    if (res != ERROR_SUCCESS) return false;

    wchar_t path[MAX_PATH] = {};
    DWORD size = sizeof(path);
    res = RegQueryValueExW(hKey, L"OverlayKit", nullptr, nullptr, reinterpret_cast<LPBYTE>(path), &size);
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS) return false;

    wchar_t currentPath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, currentPath, MAX_PATH);

    std::wstring registeredPath(path);
    if (registeredPath.length() >= 2 && registeredPath.front() == L'\"' && registeredPath.back() == L'\"') {
        registeredPath = registeredPath.substr(1, registeredPath.length() - 2);
    }

    return _wcsicmp(registeredPath.c_str(), currentPath) == 0;
}

void SetStartupEnabled(bool enable) {
    HKEY hKey;
    LONG res = RegOpenKeyExW(
        HKEY_CURRENT_USER, 
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_WRITE, &hKey
    );
    if (res != ERROR_SUCCESS) return;

    if (enable) {
        wchar_t currentPath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, currentPath, MAX_PATH);
        
        std::wstring quotedPath = L"\"" + std::wstring(currentPath) + L"\"";
        
        RegSetValueExW(
            hKey, L"OverlayKit", 0, REG_SZ,
            reinterpret_cast<const BYTE*>(quotedPath.c_str()),
            static_cast<DWORD>((quotedPath.length() + 1) * sizeof(wchar_t))
        );
    } else {
        RegDeleteValueW(hKey, L"OverlayKit");
    }

    RegCloseKey(hKey);
}

void SetCaptureExclusion(void* hwnd, bool exclude) {
    HWND wnd = reinterpret_cast<HWND>(hwnd);
    if (wnd) {
        // WDA_EXCLUDEFROMCAPTURE is 0x00000011, WDA_NONE is 0
        SetWindowDisplayAffinity(wnd, exclude ? 0x00000011 : 0);
    }
}

bool CopyBitmapToClipboard(void* hwnd, void* hBitmap) {
    HWND wnd = reinterpret_cast<HWND>(hwnd);
    HBITMAP bmp = reinterpret_cast<HBITMAP>(hBitmap);
    if (!bmp) return false;

    if (OpenClipboard(wnd)) {
        EmptyClipboard();
        SetClipboardData(CF_BITMAP, bmp);
        CloseClipboard();
        return true;
    }
    return false;
}

} // namespace Platform
