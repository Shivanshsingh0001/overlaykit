#ifndef OVERLAY_WINDOW_H
#define OVERLAY_WINDOW_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <vector>
#include "drawing_engine.h"

// Compilation flag for debug mode: set to 1 to enable title bar and border for testing
#define OVERLAY_DEBUG_MODE 0

class Toolbar;
class TrayIcon;
class SettingsWindow;
class PinnedPanel;

class OverlayWindow {
public:
    OverlayWindow();
    ~OverlayWindow();

    bool Initialize(HINSTANCE hInstance);
    void Show(bool show);
    void SetDrawMode(bool enableDrawMode);
    bool IsDrawMode() const { return m_drawMode; }
    HWND GetHWND() const { return m_hwnd; }

    DrawingEngine& GetDrawingEngine() { return m_drawingEngine; }
    Toolbar* GetToolbar() { return m_toolbar; }
    TrayIcon* GetTrayIcon() { return m_trayIcon; }
    SettingsWindow* GetSettingsWindow() { return m_settingsWindow; }

    void CaptureScreenToClipboard();
    void CommitEditText();
    void DestroyEditText();

    bool CreatePinnedPanel();
    void ClosePinnedPanel(int id);
    const std::vector<PinnedPanel*>& GetPinnedPanels() const { return m_pinnedPanels; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd;
    HWND m_hEdit;             // Text tool edit control
    int m_editX, m_editY;     // Location of active text input
    bool m_drawMode;
    COLORREF m_transparentKey;
    DrawingEngine m_drawingEngine;
    Toolbar* m_toolbar;       // Floating toolbar strip
    TrayIcon* m_trayIcon;     // System tray icon manager
    SettingsWindow* m_settingsWindow; // Settings configuration panel
    std::vector<PinnedPanel*> m_pinnedPanels; // List of active cheat sheets

    // Track virtual desktop coordinates
    int m_x;
    int m_y;
    int m_width;
    int m_height;
};

#endif // OVERLAY_WINDOW_H
