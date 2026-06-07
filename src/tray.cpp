#include "tray.h"
#include "platform.h"
#include <string>
#include <cstring>

TrayIcon::TrayIcon(HWND targetHwnd)
    : m_targetHwnd(targetHwnd), m_active(false), m_hIcon(nullptr) {
    std::memset(&m_nid, 0, sizeof(m_nid));
}

TrayIcon::~TrayIcon() {
    Destroy();
}

bool TrayIcon::Create(HINSTANCE hInstance) {
    Destroy();

    // 1. Try to load application icon from assets, fallback to default IDI_APPLICATION
    m_hIcon = reinterpret_cast<HICON>(LoadImageW(
        nullptr, L"assets/icon.ico", IMAGE_ICON, 
        0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE
    ));

    if (!m_hIcon) {
        m_hIcon = LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
    }

    // 2. Setup Notification Icon structure
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_targetHwnd;
    m_nid.uID = 1; // Unique ID for our icon
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAY_ICON_MSG;
    m_nid.hIcon = m_hIcon;
    (void)hInstance;
    
    // Set tray tooltip
    wcscpy_s(m_nid.szTip, L"OverlayKit — Press Ctrl+Alt+D to draw");

    // 3. Add notification icon
    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        m_active = true;
        return true;
    }

    return false;
}

void TrayIcon::Destroy() {
    if (m_active) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_active = false;
    }
    
    if (m_hIcon && m_hIcon != LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION))) {
        DestroyIcon(m_hIcon);
        m_hIcon = nullptr;
    }
}

void TrayIcon::ShowMenu(bool isDrawingActive) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    UINT drawFlags = MF_STRING;
    if (isDrawingActive) {
        drawFlags |= MF_CHECKED;
    }

    UINT startupFlags = MF_STRING;
    if (IsStartupEnabled()) {
        startupFlags |= MF_CHECKED;
    }

    AppendMenuW(hMenu, drawFlags, ID_TRAY_TOGGLE_DRAW, L"Draw Mode: ON/OFF");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_CLEAR, L"Clear All Annotations");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_PINNED_PANEL, L"New Pinned Panel (Cheat Sheet)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Settings...");
    AppendMenuW(hMenu, startupFlags, ID_TRAY_STARTUP, L"Start with Windows");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_ABOUT, L"About OverlayKit");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    // Win32 requirement to ensure menu closes if clicked away
    SetForegroundWindow(m_targetHwnd);

    POINT pt;
    GetCursorPos(&pt);
    
    // Popup the menu at mouse coordinates
    TrackPopupMenu(
        hMenu, 
        TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, 
        pt.x, pt.y, 
        0, m_targetHwnd, nullptr
    );
    
    DestroyMenu(hMenu);
}

bool TrayIcon::IsStartupEnabled() {
    return Platform::IsStartupEnabled();
}

void TrayIcon::SetStartupRegistry(bool enable) {
    Platform::SetStartupEnabled(enable);
}
