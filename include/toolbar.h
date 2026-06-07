#ifndef TOOLBAR_H
#define TOOLBAR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <vector>

class OverlayWindow;

enum class ToolbarOrientation {
    VERTICAL,
    HORIZONTAL
};

struct ToolbarButton {
    int id;
    Gdiplus::RectF rect;
    bool isCheckable;
    const wchar_t* tooltip;
};

class Toolbar {
public:
    explicit Toolbar(OverlayWindow* overlay);
    ~Toolbar();

    bool Initialize(HINSTANCE hInstance);
    void Show(bool show);
    void SetOrientation(ToolbarOrientation orientation);
    ToolbarOrientation GetOrientation() const { return m_orientation; }
    HWND GetHWND() const { return m_hwnd; }

    // Button IDs
    static const int BTN_PEN = 1;
    static const int BTN_HIGHLIGHTER = 2;
    static const int BTN_ERASER = 3;
    static const int BTN_LINE = 4;
    static const int BTN_ARROW = 5;
    static const int BTN_RECT = 6;
    static const int BTN_ELLIPSE = 7;
    static const int BTN_TEXT = 8;
    static const int BTN_LASER = 9;
    static const int BTN_SPOTLIGHT = 10;
    static const int BTN_HALO = 11;
    static const int BTN_FADING = 12;
    static const int BTN_WHITEBOARD = 13;
    static const int BTN_BLACKBOARD = 14;
    static const int BTN_SCREENSHOT = 15;
    static const int BTN_UNDO = 16;
    static const int BTN_CLEAR = 17;
    static const int BTN_TOGGLE_ALIGN = 18;
    static const int BTN_CLOSE = 19;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void Paint(HDC hdc, PAINTSTRUCT& ps);
    void DrawButtonIcon(Gdiplus::Graphics& graphics, int btnId, const Gdiplus::RectF& btnRect, bool isHovered, bool isSelected);
    void HandleClick(int btnId);
    
    int HitTestButton(float x, float y);
    int HitTestColorGrid(float x, float y);
    void SetupColors();
    void UpdateLayout();

    OverlayWindow* m_overlay;
    HWND m_hwnd;
    ToolbarOrientation m_orientation;
    
    std::vector<ToolbarButton> m_buttons;
    int m_hoveredBtnId;
    int m_pressedBtnId;

    // Dragging state
    bool m_isDragging;
    POINT m_dragStart;

    // 24 Color palette properties
    std::vector<Gdiplus::Color> m_presetColors;
    int m_selectedColorIndex;
    Gdiplus::RectF m_colorGridRect;
    Gdiplus::RectF m_customColorRect;
    Gdiplus::RectF m_sizeSliderRect;
    
    // Slider state
    bool m_isDraggingSlider;

    int m_x;
    int m_y;
    int m_width;
    int m_height;
};

#endif // TOOLBAR_H
