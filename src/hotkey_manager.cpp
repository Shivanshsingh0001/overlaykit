#include "hotkey_manager.h"

HotkeyManager::HotkeyManager(HWND targetHwnd)
    : m_targetHwnd(targetHwnd), m_registered(false), m_modifiers(0), m_vk(0) {}

HotkeyManager::~HotkeyManager() {
    UnregisterToggleHotkey();
}

bool HotkeyManager::RegisterToggleHotkey(UINT modifiers, UINT vk) {
    UnregisterToggleHotkey();

    m_modifiers = modifiers;
    m_vk = vk;

    // Register global hotkey tied to the target window
    if (RegisterHotKey(m_targetHwnd, TOGGLE_HOTKEY_ID, m_modifiers, m_vk)) {
        m_registered = true;
        OutputDebugStringA("HotkeyManager: Toggle hotkey registered successfully.\n");
        return true;
    }

    OutputDebugStringA("HotkeyManager: Failed to register toggle hotkey (possible conflict).\n");
    return false;
}

void HotkeyManager::UnregisterToggleHotkey() {
    if (m_registered) {
        UnregisterHotKey(m_targetHwnd, TOGGLE_HOTKEY_ID);
        m_registered = false;
        OutputDebugStringA("HotkeyManager: Toggle hotkey unregistered.\n");
    }
}
