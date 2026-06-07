#include "main.h"
#include "overlay_window.h"
#include "hotkey_manager.h"
#include <cstring>

// Set process DPI awareness to support high-DPI screens correctly
void InitializeDPIAwareness() {
    // Try SetProcessDpiAwarenessContext (Windows 10 1703+)
    typedef BOOL (WINAPI *SetProcessDpiAwarenessContextProc)(void*);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        FARPROC proc = GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (proc) {
            SetProcessDpiAwarenessContextProc setDpiCtx;
            std::memcpy(&setDpiCtx, &proc, sizeof(setDpiCtx));
            if (setDpiCtx(reinterpret_cast<void*>(-4))) {
                return;
            }
        }
    }

    // Try SetProcessDpiAwareness (Windows 8.1+)
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        typedef HRESULT (WINAPI *SetProcessDpiAwarenessProc)(int);
        FARPROC proc = GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (proc) {
            SetProcessDpiAwarenessProc setDpi;
            std::memcpy(&setDpi, &proc, sizeof(setDpi));
            setDpi(2);
        }
        FreeLibrary(hShcore);
        return;
    }

    // Try SetProcessDPIAware (Windows Vista+)
    SetProcessDPIAware();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    InitializeDPIAwareness();

    // Initialize GDI+
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    {
        OverlayWindow overlay;
        if (!overlay.Initialize(hInstance)) {
            MessageBoxW(nullptr, L"Failed to initialize overlay window.", L"Error", MB_OK | MB_ICONERROR);
            Gdiplus::GdiplusShutdown(gdiplusToken);
            return -1;
        }

        // Initialize Hotkey Manager
        HotkeyManager hotkeyMgr(overlay.GetHWND());
        if (!hotkeyMgr.RegisterToggleHotkey(MOD_CONTROL | MOD_ALT, 'D')) {
            MessageBoxW(
                nullptr,
                L"Failed to register global hotkey Ctrl+Alt+D.\nIt might be in use by another application.",
                L"Hotkey Warning",
                MB_OK | MB_ICONWARNING
            );
        }

        // Start overlay window in drawing mode directly on startup
        overlay.Show(true);
        overlay.SetDrawMode(true);

        // Message loop
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}
