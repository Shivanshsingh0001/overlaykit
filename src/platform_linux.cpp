#include "platform.h"

namespace Platform {

ScreenBounds GetVirtualScreenBounds() {
    // Stub bounds for X11/Wayland virtual screen
    ScreenBounds bounds;
    bounds.x = 0;
    bounds.y = 0;
    bounds.width = 1920;  // Standard fallback
    bounds.height = 1080; // Standard fallback
    return bounds;
}

bool IsStartupEnabled() {
    // Stub for Linux desktop autostart entry check
    return false;
}

void SetStartupEnabled(bool enable) {
    // Stub for writing ~/.config/autostart/overlaykit.desktop
    (void)enable;
}

void SetCaptureExclusion(void* hwnd, bool exclude) {
    // Stub for X11 screen capture exclusion / Wayland secure window properties
    (void)hwnd;
    (void)exclude;
}

bool CopyBitmapToClipboard(void* hwnd, void* hBitmap) {
    // Stub for X11/GTK clipboard selection set
    (void)hwnd;
    (void)hBitmap;
    return false;
}

} // namespace Platform
