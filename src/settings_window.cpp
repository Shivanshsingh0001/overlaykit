#include "settings_window.h"
#include "tray.h"
#include "platform.h"
#include <algorithm>
#include <vector>

// Subclass procedure for hotkey inputs
static WNDPROC g_OriginalHotkeyEditProc = nullptr;

LRESULT CALLBACK HotkeyEditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        // Prevent Tab or Escape from being consumed if needed, but allow remapping
        if (wParam == VK_TAB) {
            // Forward Tab key to let user focus out
            return CallWindowProcW(g_OriginalHotkeyEditProc, hwnd, msg, wParam, lParam);
        }

        std::wstring combo = L"";
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (ctrl) combo += L"Ctrl+";
        if (alt) combo += L"Alt+";
        if (shift) combo += L"Shift+";

        if (wParam >= 'A' && wParam <= 'Z') {
            combo += static_cast<wchar_t>(wParam);
        } else if (wParam >= '0' && wParam <= '9') {
            combo += static_cast<wchar_t>(wParam);
        } else if (wParam >= VK_F1 && wParam <= VK_F12) {
            wchar_t fKey[8];
            swprintf_s(fKey, L"F%d", static_cast<int>(wParam - VK_F1 + 1));
            combo += fKey;
        } else {
            // Try to map other keys
            wchar_t keyName[64] = {};
            // Translate scan code from lParam
            LONG scanCode = (lParam >> 16) & 0xFF;
            if (scanCode != 0) {
                GetKeyNameTextW(scanCode << 16, keyName, 64);
                combo += keyName;
            }
        }

        // Write combo representation to Edit box
        SetWindowTextW(hwnd, combo.c_str());
        return 0;
    }
    return CallWindowProcW(g_OriginalHotkeyEditProc, hwnd, msg, wParam, lParam);
}

SettingsWindow::SettingsWindow()
    : m_hwnd(nullptr), m_ownerHwnd(nullptr),
      m_hEditToggleHotkey(nullptr), m_hEditClearHotkey(nullptr), m_hEditUndoHotkey(nullptr),
      m_hComboDefaultTool(nullptr), m_hCheckStartup(nullptr),
      m_hEditLicenseName(nullptr), m_hEditLicenseKey(nullptr), m_hStaticLicenseStatus(nullptr) {}

SettingsWindow::~SettingsWindow() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool SettingsWindow::Create(HWND ownerHwnd, HINSTANCE hInstance) {
    m_ownerHwnd = ownerHwnd;

    // Load active config to temp struct
    ConfigManager::Load(m_tempConfig);

    // Register class for settings dialog
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SettingsWindow::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"OverlayKit_Settings_Class";

    if (!RegisterClassExW(&wc)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    // Center dialog on owner window
    RECT ownerRect;
    GetWindowRect(ownerHwnd, &ownerRect);
    int ownerWidth = ownerRect.right - ownerRect.left;
    int ownerHeight = ownerRect.bottom - ownerRect.top;
    
    int dlgWidth = 380;
    int dlgHeight = 480;
    int x = ownerRect.left + (ownerWidth - dlgWidth) / 2;
    int y = ownerRect.top + (ownerHeight - dlgHeight) / 2;

    // Create popup window with dialog style
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        L"OverlayKit_Settings_Class",
        L"OverlayKit Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, dlgWidth, dlgHeight,
        ownerHwnd, nullptr, hInstance, this
    );

    if (!m_hwnd) return false;

    // Create child controls
    // Standard system font
    HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

    // 1. Title static
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"OverlayKit Configuration", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 10, 360, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // 2. Hotkey labels & inputs
    HWND hL1 = CreateWindowExW(0, L"STATIC", L"Toggle Draw Mode:", WS_CHILD | WS_VISIBLE, 20, 50, 120, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hL1, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    m_hEditToggleHotkey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 150, 50, 190, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hEditToggleHotkey, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hL2 = CreateWindowExW(0, L"STATIC", L"Clear Canvas:", WS_CHILD | WS_VISIBLE, 20, 85, 120, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hL2, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    m_hEditClearHotkey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 150, 85, 190, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hEditClearHotkey, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hL3 = CreateWindowExW(0, L"STATIC", L"Undo Last Stroke:", WS_CHILD | WS_VISIBLE, 20, 120, 120, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hL3, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    m_hEditUndoHotkey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 150, 120, 190, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hEditUndoHotkey, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Subclass the hotkey inputs to intercept shortcuts
    g_OriginalHotkeyEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        m_hEditToggleHotkey, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HotkeyEditSubclass)
    ));
    SetWindowLongPtrW(m_hEditClearHotkey, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HotkeyEditSubclass));
    SetWindowLongPtrW(m_hEditUndoHotkey, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HotkeyEditSubclass));

    // 3. Default Tool selection
    HWND hL4 = CreateWindowExW(0, L"STATIC", L"Default Tool:", WS_CHILD | WS_VISIBLE, 20, 155, 120, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hL4, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    m_hComboDefaultTool = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 150, 155, 190, 100, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hComboDefaultTool, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    // Populate tools
    SendMessageW(m_hComboDefaultTool, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Pen"));
    SendMessageW(m_hComboDefaultTool, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Highlighter"));
    SendMessageW(m_hComboDefaultTool, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Laser Pointer"));
    SendMessageW(m_hComboDefaultTool, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Spotlight"));

    // 4. Windows Startup Checkbox
    m_hCheckStartup = CreateWindowExW(0, L"BUTTON", L"Launch OverlayKit automatically at Windows startup", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 190, 330, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hCheckStartup, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // 5. Group line for Licensing
    HWND hLicGrp = CreateWindowExW(0, L"BUTTON", L"License Management", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 230, 340, 140, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hLicGrp, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hLName = CreateWindowExW(0, L"STATIC", L"Licensee Name:", WS_CHILD | WS_VISIBLE, 30, 260, 100, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hLName, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    m_hEditLicenseName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 140, 260, 200, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hEditLicenseName, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hLKey = CreateWindowExW(0, L"STATIC", L"License Key:", WS_CHILD | WS_VISIBLE, 30, 295, 100, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(hLKey, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    m_hEditLicenseKey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 140, 295, 200, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hEditLicenseKey, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hBtnValidate = CreateWindowExW(0, L"BUTTON", L"Verify Offline", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 140, 330, 100, 25, m_hwnd, reinterpret_cast<HMENU>(3008), hInstance, nullptr);
    SendMessageW(hBtnValidate, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    m_hStaticLicenseStatus = CreateWindowExW(0, L"STATIC", L"Unlicensed", WS_CHILD | WS_VISIBLE, 250, 335, 90, 20, m_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(m_hStaticLicenseStatus, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // 6. Action buttons
    HWND hBtnSave = CreateWindowExW(0, L"BUTTON", L"Save Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 40, 390, 100, 30, m_hwnd, reinterpret_cast<HMENU>(3009), hInstance, nullptr);
    SendMessageW(hBtnSave, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 150, 390, 90, 30, m_hwnd, reinterpret_cast<HMENU>(3010), hInstance, nullptr);
    SendMessageW(hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    HWND hBtnDefaults = CreateWindowExW(0, L"BUTTON", L"Restore Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 250, 390, 100, 30, m_hwnd, reinterpret_cast<HMENU>(3011), hInstance, nullptr);
    SendMessageW(hBtnDefaults, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Fill UI inputs with config settings
    LoadConfigToUI();

    return true;
}

void SettingsWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(m_hwnd);
    }
}

void SettingsWindow::LoadConfigToUI() {
    SetWindowTextW(m_hEditToggleHotkey, m_tempConfig.hotkey_toggle.c_str());
    SetWindowTextW(m_hEditClearHotkey, m_tempConfig.hotkey_clear.c_str());
    SetWindowTextW(m_hEditUndoHotkey, m_tempConfig.hotkey_undo.c_str());

    // Set combo index
    int index = 0;
    if (m_tempConfig.default_tool == L"highlighter") index = 1;
    else if (m_tempConfig.default_tool == L"laser") index = 2;
    else if (m_tempConfig.default_tool == L"spotlight") index = 3;
    SendMessageW(m_hComboDefaultTool, CB_SETCURSEL, index, 0);

    // Checkbox
    SendMessageW(m_hCheckStartup, BM_SETCHECK, m_tempConfig.start_with_windows ? BST_CHECKED : BST_UNCHECKED, 0);

    // License
    SetWindowTextW(m_hEditLicenseName, m_tempConfig.license_name.c_str());
    SetWindowTextW(m_hEditLicenseKey, m_tempConfig.license_key.c_str());

    if (m_tempConfig.is_licensed) {
        SetWindowTextW(m_hStaticLicenseStatus, L"Activated");
    } else {
        SetWindowTextW(m_hStaticLicenseStatus, L"Unlicensed");
    }
}

void SettingsWindow::SaveUIToConfig() {
    // Read hotkeys
    wchar_t buf[256];
    
    GetWindowTextW(m_hEditToggleHotkey, buf, 256);
    m_tempConfig.hotkey_toggle = buf;
    
    GetWindowTextW(m_hEditClearHotkey, buf, 256);
    m_tempConfig.hotkey_clear = buf;

    GetWindowTextW(m_hEditUndoHotkey, buf, 256);
    m_tempConfig.hotkey_undo = buf;

    // Read combo tool
    int toolIdx = static_cast<int>(SendMessageW(m_hComboDefaultTool, CB_GETCURSEL, 0, 0));
    if (toolIdx == 1) m_tempConfig.default_tool = L"highlighter";
    else if (toolIdx == 2) m_tempConfig.default_tool = L"laser";
    else if (toolIdx == 3) m_tempConfig.default_tool = L"spotlight";
    else m_tempConfig.default_tool = L"pen";

    // Read checkbox
    m_tempConfig.start_with_windows = (SendMessageW(m_hCheckStartup, BM_GETCHECK, 0, 0) == BST_CHECKED);

    // Read license
    GetWindowTextW(m_hEditLicenseName, buf, 256);
    m_tempConfig.license_name = buf;

    GetWindowTextW(m_hEditLicenseKey, buf, 256);
    m_tempConfig.license_key = buf;

    // Recalculate license
    m_tempConfig.is_licensed = ConfigManager::VerifyLicenseKey(m_tempConfig.license_name, m_tempConfig.license_key);

    // Save using ConfigManager
    ConfigManager::Save(m_tempConfig);
}

void SettingsWindow::ResetToDefaults() {
    AppConfig defaults; // Create temporary default struct
    m_tempConfig = defaults;
    LoadConfigToUI();
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    SettingsWindow* pThis = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT SettingsWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);

            if (wmId == 3008) { // Verify offline button
                wchar_t nameBuf[256] = {};
                wchar_t keyBuf[256] = {};
                GetWindowTextW(m_hEditLicenseName, nameBuf, 256);
                GetWindowTextW(m_hEditLicenseKey, keyBuf, 256);

                bool valid = ConfigManager::VerifyLicenseKey(nameBuf, keyBuf);
                SetWindowTextW(m_hStaticLicenseStatus, valid ? L"Activated" : L"Invalid Key");
                return 0;
            }
            else if (wmId == 3009) { // Save button
                SaveUIToConfig();
                
                // Toggle startup registry settings based on config choice
                // We'll communicate this to the tray manager
                Platform::SetStartupEnabled(m_tempConfig.start_with_windows);

                // Show notification in debug logs
                OutputDebugStringA("SettingsWindow: Configuration saved successfully.\n");

                // Hide settings window
                Show(false);
                return 0;
            }
            else if (wmId == 3010) { // Cancel button
                // Hide settings window without saving
                Show(false);
                return 0;
            }
            else if (wmId == 3011) { // Restore Defaults button
                ResetToDefaults();
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            Show(false);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
