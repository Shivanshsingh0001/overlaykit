#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

class HotkeyManager {
public:
    explicit HotkeyManager(HWND targetHwnd);
    ~HotkeyManager();

    // Register a global hotkey. Returns false if registration fails (e.g. key conflict).
    bool RegisterToggleHotkey(UINT modifiers, UINT vk);
    void UnregisterToggleHotkey();
    bool IsRegistered() const { return m_registered; }

    static const int TOGGLE_HOTKEY_ID = 1001;

private:
    HWND m_targetHwnd;
    bool m_registered;
    UINT m_modifiers;
    UINT m_vk;
};

#endif // HOTKEY_MANAGER_H
