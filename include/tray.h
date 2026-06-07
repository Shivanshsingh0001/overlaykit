#ifndef TRAY_H
#define TRAY_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>

class TrayIcon {
public:
    explicit TrayIcon(HWND targetHwnd);
    ~TrayIcon();

    bool Create(HINSTANCE hInstance);
    void Destroy();
    
    void ShowMenu(bool isDrawingActive);
    void SetStartupRegistry(bool enable);
    bool IsStartupEnabled();

    // Context Menu Command IDs
    static const int ID_TRAY_TOGGLE_DRAW = 2001;
    static const int ID_TRAY_CLEAR = 2002;
    static const int ID_TRAY_SETTINGS = 2003;
    static const int ID_TRAY_STARTUP = 2004;
    static const int ID_TRAY_ABOUT = 2005;
    static const int ID_TRAY_EXIT = 2006;
    static const int ID_TRAY_PINNED_PANEL = 2007;

    // Custom window message for tray interactions
    static const UINT WM_TRAY_ICON_MSG = WM_USER + 200;

private:
    HWND m_targetHwnd;
    NOTIFYICONDATAW m_nid;
    bool m_active;
    HICON m_hIcon;
};

#endif // TRAY_H
