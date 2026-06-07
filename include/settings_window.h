#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "config_manager.h"

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();

    bool Create(HWND ownerHwnd, HINSTANCE hInstance);
    void Show(bool show);
    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void LoadConfigToUI();
    void SaveUIToConfig();
    void ResetToDefaults();

    HWND m_hwnd;
    HWND m_ownerHwnd;
    AppConfig m_tempConfig;

    // Control handles
    HWND m_hEditToggleHotkey;
    HWND m_hEditClearHotkey;
    HWND m_hEditUndoHotkey;
    HWND m_hComboDefaultTool;
    HWND m_hCheckStartup;
    HWND m_hEditLicenseName;
    HWND m_hEditLicenseKey;
    HWND m_hStaticLicenseStatus;
};

#endif // SETTINGS_WINDOW_H
