#include "overlay_window.h"
#include "toolbar.h"
#include "tray.h"
#include "settings_window.h"
#include "pinned_panel.h"
#include "platform.h"

// Define the user data slot wrapper
#ifndef GWLP_USERDATA
#define GWLP_USERDATA (-21)
#endif

// Pointer to the original EDIT class window procedure
static WNDPROC g_OriginalEditProc = nullptr;

OverlayWindow::OverlayWindow()
    : m_hwnd(nullptr), m_hEdit(nullptr), m_editX(0), m_editY(0),
      m_drawMode(false), m_transparentKey(RGB(0, 0, 0)),
      m_toolbar(nullptr), m_trayIcon(nullptr), m_settingsWindow(nullptr) {}

OverlayWindow::~OverlayWindow() {
    if (m_hwnd) {
        KillTimer(m_hwnd, 1);
        DestroyEditText();
        if (m_toolbar) {
            delete m_toolbar;
            m_toolbar = nullptr;
        }
        if (m_trayIcon) {
            delete m_trayIcon;
            m_trayIcon = nullptr;
        }
        if (m_settingsWindow) {
            delete m_settingsWindow;
            m_settingsWindow = nullptr;
        }
        for (auto panel : m_pinnedPanels) {
            delete panel;
        }
        m_pinnedPanels.clear();
        m_drawingEngine.Shutdown();
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool OverlayWindow::Initialize(HINSTANCE hInstance) {
    // 1. Get virtual monitor coordinates to cover all monitors
    ScreenBounds bounds = Platform::GetVirtualScreenBounds();
    m_x = bounds.x;
    m_y = bounds.y;
    m_width = bounds.width;
    m_height = bounds.height;

    // 2. Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayWindow::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"OverlayKit_Overlay_Class";

    if (!RegisterClassExW(&wc)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    // 3. Set window style based on debug mode compile flag
    DWORD dwStyle = WS_POPUP;
    DWORD dwExStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;

#if OVERLAY_DEBUG_MODE
    dwStyle = WS_OVERLAPPEDWINDOW;
    dwExStyle = WS_EX_TOPMOST;
#else
    dwExStyle |= WS_EX_TRANSPARENT;
#endif

    // 4. Create the window
    m_hwnd = CreateWindowExW(
        dwExStyle,
        L"OverlayKit_Overlay_Class",
        L"OverlayKit Fullscreen Overlay",
        dwStyle,
        m_x, m_y, m_width, m_height,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) {
        return false;
    }

#if !OVERLAY_DEBUG_MODE
    // 5. Apply layered window attributes (color keying)
    if (!SetLayeredWindowAttributes(m_hwnd, m_transparentKey, 0, LWA_COLORKEY)) {
        return false;
    }
#endif

    // 6. Initialize the drawing engine with window dimension context
    if (!m_drawingEngine.Initialize(m_hwnd, m_width, m_height)) {
        return false;
    }

    // 7. Initialize Toolbar
    m_toolbar = new Toolbar(this);
    if (!m_toolbar->Initialize(hInstance)) {
        return false;
    }

    // 8. Initialize Tray Icon
    m_trayIcon = new TrayIcon(m_hwnd);
    if (!m_trayIcon->Create(hInstance)) {
        return false;
    }

    // 9. Initialize Settings Window
    m_settingsWindow = new SettingsWindow();
    if (!m_settingsWindow->Create(m_hwnd, hInstance)) {
        return false;
    }

    // 10. Start the timer for fading ink ticks (approx 30fps)
    SetTimer(m_hwnd, 1, 30, nullptr);

    return true;
}

void OverlayWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(m_hwnd);
    }
}

void OverlayWindow::SetDrawMode(bool enableDrawMode) {
    if (!m_hwnd) return;

    m_drawMode = enableDrawMode;

    // If leaving draw mode, commit any active editing text
    if (!m_drawMode) {
        CommitEditText();
    }

    // Toggle Toolbar visibility based on draw mode
    if (m_toolbar) {
        m_toolbar->Show(m_drawMode);
    }

#if !OVERLAY_DEBUG_MODE
    LONG_PTR exStyle = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);
    if (m_drawMode) {
        exStyle &= ~WS_EX_TRANSPARENT;
        SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, exStyle);
        SetClassLongPtrW(m_hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(nullptr, IDC_CROSS)));
        SetCursor(LoadCursor(nullptr, IDC_CROSS));
    } else {
        exStyle |= WS_EX_TRANSPARENT;
        SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, exStyle);
        SetClassLongPtrW(m_hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(nullptr, IDC_ARROW)));
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }

    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
}

void OverlayWindow::CommitEditText() {
    if (!m_hEdit) return;

    int len = GetWindowTextLengthW(m_hEdit);
    if (len > 0) {
        std::wstring text(len, L'\0');
        GetWindowTextW(m_hEdit, &text[0], len + 1);

        float fontSize = 12.0f + m_drawingEngine.GetBrushWidth() * 2.0f;
        m_drawingEngine.CommitText(text, static_cast<float>(m_editX), static_cast<float>(m_editY), L"Arial", fontSize);
    }

    DestroyEditText();
}

void OverlayWindow::DestroyEditText() {
    if (m_hEdit) {
        // Remove subclassing and destroy
        SetWindowLongPtrW(m_hEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_OriginalEditProc));
        DestroyWindow(m_hEdit);
        m_hEdit = nullptr;
        SetFocus(m_hwnd);
    }
}

void OverlayWindow::CaptureScreenToClipboard() {
    // 1. Get virtual screen bounds (covering all monitors)
    ScreenBounds bounds = Platform::GetVirtualScreenBounds();
    int x = bounds.x;
    int y = bounds.y;
    int w = bounds.width;
    int h = bounds.height;

    // 2. Perform GDI screen capture block
    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(hMemoryDC, hBitmap));

    // Capture desktop contents directly
    BitBlt(hMemoryDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY);

    // Restore DC context
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    // 3. Put bitmap on clipboard
    if (Platform::CopyBitmapToClipboard(m_hwnd, hBitmap)) {
        OutputDebugStringA("OverlayKit: Screenshot successfully copied to clipboard.\n");
    } else {
        DeleteObject(hBitmap);
    }
}

LRESULT CALLBACK OverlayWindow::EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            // Commit text unless Shift is pressed (multiline text)
            if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
                PostMessageW(GetParent(hwnd), WM_USER + 100, 0, 0); // Commit text trigger
                return 0;
            }
        } else if (wParam == VK_ESCAPE) {
            PostMessageW(GetParent(hwnd), WM_USER + 101, 0, 0); // Cancel text trigger
            return 0;
        }
    }
    // Commit on focus loss
    if (msg == WM_KILLFOCUS) {
        PostMessageW(GetParent(hwnd), WM_USER + 100, 0, 0);
    }
    return CallWindowProcW(g_OriginalEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OverlayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    OverlayWindow* pThis = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        m_hwnd = hwnd;
    }

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            HBRUSH hBrush = CreateSolidBrush(m_transparentKey);
            FillRect(hdc, &ps.rcPaint, hBrush);
            DeleteObject(hBrush);

            Gdiplus::Graphics graphics(hdc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            m_drawingEngine.Draw(graphics);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN:
            if (m_drawMode) {
                float x = static_cast<float>(static_cast<short>(LOWORD(lParam)));
                float y = static_cast<float>(static_cast<short>(HIWORD(lParam)));

                // Text tool handling
                if (m_drawingEngine.GetTool() == ToolType::TEXT) {
                    if (m_hEdit) {
                        CommitEditText();
                    }

                    m_editX = static_cast<int>(x);
                    m_editY = static_cast<int>(y);

                    m_hEdit = CreateWindowExW(
                        0, L"EDIT", L"",
                        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_MULTILINE,
                        m_editX, m_editY, 250, 40,
                        hwnd, nullptr,
                        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)), nullptr
                    );

                    if (m_hEdit) {
                        float size = 12.0f + m_drawingEngine.GetBrushWidth() * 2.0f;
                        HFONT hFont = CreateFontW(
                            static_cast<int>(size * 1.3f), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, L"Arial"
                        );
                        SendMessageW(m_hEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

                        g_OriginalEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
                            m_hEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OverlayWindow::EditSubclassProc)
                        ));

                        SetFocus(m_hEdit);
                    }
                } else {
                    // Standard pen strokes
                    SetCapture(hwnd);
                    m_drawingEngine.StartStroke(x, y);
                }
                return 0;
            }
            break;
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(static_cast<short>(LOWORD(lParam)));
            float y = static_cast<float>(static_cast<short>(HIWORD(lParam)));

            m_drawingEngine.UpdateCursorPosition(x, y);

            if (m_drawMode && m_drawingEngine.IsDrawing()) {
                m_drawingEngine.AddPointToStroke(x, y);
            }
            return 0;
        }
        case WM_LBUTTONUP:
            if (m_drawMode && m_drawingEngine.IsDrawing()) {
                m_drawingEngine.EndStroke();
                ReleaseCapture();
                return 0;
            }
            break;
        case WM_KEYDOWN: {
            bool ctrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            } else if (ctrlPressed && wParam == 'Z') {
                m_drawingEngine.Undo();
                return 0;
            } else if (ctrlPressed && shiftPressed && wParam == 'C') {
                m_drawingEngine.Clear();
                return 0;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            m_drawingEngine.Resize(w, h);
            return 0;
        }
        case WM_TIMER:
            if (wParam == 1) {
                m_drawingEngine.UpdateFadingStrokes();
                return 0;
            }
            break;
        case WM_USER + 100: // Commit text box message from subclass
            CommitEditText();
            return 0;
        case WM_USER + 101: // Cancel text box message from subclass
            DestroyEditText();
            return 0;
        case WM_USER + 200: { // TrayIcon::WM_TRAY_ICON_MSG
            if (lParam == WM_RBUTTONUP) {
                if (m_trayIcon) {
                    m_trayIcon->ShowMenu(m_drawMode);
                }
                return 0;
            } else if (lParam == WM_LBUTTONUP) {
                SetDrawMode(!IsDrawMode());
                return 0;
            }
            break;
        }
        case WM_USER + 300: { // PinnedPanel close notification
            int id = static_cast<int>(wParam);
            ClosePinnedPanel(id);
            return 0;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case 2001: // TrayIcon::ID_TRAY_TOGGLE_DRAW
                    SetDrawMode(!IsDrawMode());
                    break;
                case 2002: // TrayIcon::ID_TRAY_CLEAR
                    m_drawingEngine.Clear();
                    break;
                case 2003: // TrayIcon::ID_TRAY_SETTINGS
                    if (m_settingsWindow) {
                        m_settingsWindow->Show(true);
                    }
                    break;
                case 2004: // TrayIcon::ID_TRAY_STARTUP
                    if (m_trayIcon) {
                        bool enabled = m_trayIcon->IsStartupEnabled();
                        m_trayIcon->SetStartupRegistry(!enabled);
                    }
                    break;
                case 2005: // TrayIcon::ID_TRAY_ABOUT
                    MessageBoxW(hwnd, L"OverlayKit v1.0.0\n\nA lightweight, always-on-top transparent presentation overlay tool.\n\nBuilt using native C++ and Win32 GDI+.", L"About OverlayKit", MB_OK | MB_ICONINFORMATION);
                    break;
                case 2006: // TrayIcon::ID_TRAY_EXIT
                    DestroyWindow(hwnd);
                    break;
                case 2007: // TrayIcon::ID_TRAY_PINNED_PANEL
                    CreatePinnedPanel();
                    break;
            }
            return 0;
        }
        case WM_HOTKEY:
            if (wParam == 1001) { // HotkeyManager::TOGGLE_HOTKEY_ID
                SetDrawMode(!IsDrawMode());
                OutputDebugStringA(m_drawMode ? "OverlayKit: Draw Mode ACTIVE\n" : "OverlayKit: Draw Mode PASSTHROUGH\n");
                return 0;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool OverlayWindow::CreatePinnedPanel() {
    if (m_pinnedPanels.size() >= 5) {
        MessageBoxW(m_hwnd, L"Maximum of 5 Pinned Panels can be open at a time.", L"Limit Reached", MB_OK | MB_ICONWARNING);
        return false;
    }

    // Find first unused ID in 1..5
    int newId = 1;
    for (int id = 1; id <= 5; ++id) {
        bool used = false;
        for (auto p : m_pinnedPanels) {
            if (p->GetId() == id) {
                used = true;
                break;
            }
        }
        if (!used) {
            newId = id;
            break;
        }
    }

    PinnedPanel* panel = new PinnedPanel();
    // Cascade position based on count
    int offset = static_cast<int>(m_pinnedPanels.size()) * 30;
    int px = 200 + offset;
    int py = 200 + offset;
    int pw = 320;
    int ph = 240;

    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(m_hwnd, GWLP_HINSTANCE));
    if (panel->Create(m_hwnd, hInstance, newId, px, py, pw, ph)) {
        m_pinnedPanels.push_back(panel);
        panel->Show(true);
        OutputDebugStringA("OverlayWindow: Created Pinned Panel\n");
        return true;
    }

    delete panel;
    return false;
}

void OverlayWindow::ClosePinnedPanel(int id) {
    for (auto it = m_pinnedPanels.begin(); it != m_pinnedPanels.end(); ++it) {
        if ((*it)->GetId() == id) {
            PinnedPanel* panel = *it;
            m_pinnedPanels.erase(it);
            delete panel;
            OutputDebugStringA("OverlayWindow: Closed Pinned Panel\n");
            break;
        }
    }
}
