#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

struct ScreenBounds {
    int x;
    int y;
    int width;
    int height;
};

namespace Platform {
    // Screen / Monitor information
    ScreenBounds GetVirtualScreenBounds();

    // Startup / Autostart Registry / Desktop Entry
    bool IsStartupEnabled();
    void SetStartupEnabled(bool enable);

    // Display / Capture affinity exclusion
    void SetCaptureExclusion(void* hwnd, bool exclude);

    // Clipboard helpers
    bool CopyBitmapToClipboard(void* hwnd, void* hBitmap);
}

#endif // PLATFORM_H
