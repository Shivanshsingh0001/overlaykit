#ifndef PINNED_PANEL_H
#define PINNED_PANEL_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>
#include <objidl.h>
#include <gdiplus.h>

class PinnedPanel {
public:
    PinnedPanel();
    ~PinnedPanel();

    bool Create(HWND ownerHwnd, HINSTANCE hInstance, int id, int x, int y, int w, int h);
    void Show(bool show);
    HWND GetHWND() const { return m_hwnd; }
    int GetId() const { return m_id; }

    bool LoadFile(const std::wstring& filePath);
    void ClearContent();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd;
    HWND m_ownerHwnd;
    int m_id;
    int m_scrollY;
    int m_maxScrollY;

    // Loaded content
    std::wstring m_filePath;
    bool m_isImage;
    Gdiplus::Image* m_image;
    std::wstring m_text;
};

#endif // PINNED_PANEL_H
