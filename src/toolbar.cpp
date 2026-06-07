#include "toolbar.h"
#include "overlay_window.h"
#include <commdlg.h>
#include <algorithm>
#include <cmath>

Toolbar::Toolbar(OverlayWindow* overlay)
    : m_overlay(overlay), m_hwnd(nullptr), m_orientation(ToolbarOrientation::VERTICAL),
      m_hoveredBtnId(0), m_pressedBtnId(0), m_isDragging(false),
      m_selectedColorIndex(0), m_isDraggingSlider(false),
      m_x(100), m_y(200), m_width(76), m_height(564) {
    m_dragStart.x = 0; m_dragStart.y = 0;
    SetupColors();
}

Toolbar::~Toolbar() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void Toolbar::SetupColors() {
    // 24 premium presentation colors
    m_presetColors = {
        Gdiplus::Color(255, 255, 0, 0),     // Red
        Gdiplus::Color(255, 0, 200, 0),     // Green
        Gdiplus::Color(255, 0, 100, 255),   // Blue
        Gdiplus::Color(255, 255, 200, 0),   // Yellow
        Gdiplus::Color(255, 255, 0, 255),   // Magenta
        Gdiplus::Color(255, 0, 255, 255),   // Cyan
        
        Gdiplus::Color(255, 255, 128, 0),   // Orange
        Gdiplus::Color(255, 128, 0, 255),   // Purple
        Gdiplus::Color(255, 255, 100, 150), // Pink
        Gdiplus::Color(255, 0, 128, 128),   // Teal
        Gdiplus::Color(255, 128, 64, 0),    // Brown
        Gdiplus::Color(255, 128, 128, 128), // Gray
        
        Gdiplus::Color(255, 255, 255, 255), // White
        Gdiplus::Color(255, 1, 1, 1),       // Black (off-black)
        Gdiplus::Color(255, 255, 150, 150), // Light Red
        Gdiplus::Color(255, 150, 255, 150), // Light Green
        Gdiplus::Color(255, 150, 200, 255), // Light Blue
        Gdiplus::Color(255, 255, 255, 150), // Light Yellow
        
        Gdiplus::Color(255, 255, 150, 255), // Light Magenta
        Gdiplus::Color(255, 150, 255, 255), // Light Cyan
        Gdiplus::Color(255, 255, 200, 150), // Light Orange
        Gdiplus::Color(255, 200, 150, 255), // Light Purple
        Gdiplus::Color(255, 150, 220, 220), // Light Teal
        Gdiplus::Color(255, 200, 200, 200)  // Light Gray
    };
}

bool Toolbar::Initialize(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Toolbar::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // Double buffered manual paint
    wc.lpszClassName = L"OverlayKit_Toolbar_Class";

    if (!RegisterClassExW(&wc)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    // Initialize layout positions
    UpdateLayout();

    // Create layered, topmost toolbar window
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        L"OverlayKit_Toolbar_Class",
        L"OverlayKit Toolbar",
        WS_POPUP,
        m_x, m_y, m_width, m_height,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) return false;

    // Apply alpha opacity (217 / 255 = 85%)
    SetLayeredWindowAttributes(m_hwnd, 0, 217, LWA_ALPHA);

    return true;
}

void Toolbar::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(m_hwnd);
    }
}

void Toolbar::SetOrientation(ToolbarOrientation orientation) {
    if (m_orientation == orientation) return;
    m_orientation = orientation;

    // Swap dimensions
    std::swap(m_width, m_height);
    UpdateLayout();

    if (m_hwnd) {
        SetWindowPos(m_hwnd, HWND_TOPMOST, m_x, m_y, m_width, m_height, SWP_NOACTIVATE | SWP_FRAMECHANGED);
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void Toolbar::UpdateLayout() {
    m_buttons.clear();

    // Define the list of buttons
    std::vector<std::pair<int, const wchar_t*>> btns = {
        { BTN_PEN, L"Pen" },
        { BTN_HIGHLIGHTER, L"Highlighter" },
        { BTN_ERASER, L"Eraser" },
        { BTN_LINE, L"Line" },
        { BTN_ARROW, L"Arrow" },
        { BTN_RECT, L"Rectangle" },
        { BTN_ELLIPSE, L"Ellipse" },
        { BTN_TEXT, L"Text Tool" },
        { BTN_LASER, L"Laser Pointer" },
        { BTN_SPOTLIGHT, L"Spotlight" },
        { BTN_HALO, L"Cursor Halo" },
        { BTN_FADING, L"Fading Ink" },
        { BTN_WHITEBOARD, L"Whiteboard" },
        { BTN_BLACKBOARD, L"Blackboard" },
        { BTN_SCREENSHOT, L"Take Screenshot" },
        { BTN_UNDO, L"Undo" },
        { BTN_CLEAR, L"Clear All" },
        { BTN_TOGGLE_ALIGN, L"Change Alignment" },
        { BTN_CLOSE, L"Exit Drawing" }
    };

    const float btnSize = 28.0f;
    const float cellPadding = 4.0f;
    const float cellSize = btnSize + cellPadding; // 32.0f

    if (m_orientation == ToolbarOrientation::VERTICAL) {
        m_width = 76; // (2 columns * 32px) + 12px margin
        
        // Layout buttons in 2 columns, N rows starting below drag handle (y=16)
        float startX = 6.0f;
        float startY = 16.0f;

        for (size_t i = 0; i < btns.size(); ++i) {
            float x = startX + (i % 2) * cellSize;
            float y = startY + (i / 2) * cellSize;

            ToolbarButton btn;
            btn.id = btns[i].first;
            btn.rect = Gdiplus::RectF(x, y, btnSize, btnSize);
            btn.isCheckable = (btn.id <= BTN_SPOTLIGHT || btn.id == BTN_HALO || btn.id == BTN_FADING || btn.id == BTN_WHITEBOARD || btn.id == BTN_BLACKBOARD);
            btn.tooltip = btns[i].second;
            m_buttons.push_back(btn);
        }

        // Color Picker Grid: 2 columns, 12 rows centered below buttons
        float colorStartY = startY + ((btns.size() + 1) / 2) * cellSize + 8.0f;
        m_colorGridRect = Gdiplus::RectF(startX + 12.0f, colorStartY, 24.0f, 144.0f);

        // Custom Color dialog button below grid
        float customStartY = colorStartY + 144.0f + 6.0f;
        m_customColorRect = Gdiplus::RectF(startX + 12.0f, customStartY, 24.0f, 24.0f);

        // Vertical Size Slider centered below custom color
        float sliderStartY = customStartY + 24.0f + 12.0f;
        m_sizeSliderRect = Gdiplus::RectF(startX + 14.0f, sliderStartY, 20.0f, 60.0f);

        m_height = static_cast<int>(sliderStartY + 60.0f + 12.0f);
    } else {
        m_height = 76; // (2 rows * 32px) + 12px margin
        
        // Layout buttons in 2 rows, N columns starting past drag handle (x=16)
        float startX = 16.0f;
        float startY = 6.0f;

        for (size_t i = 0; i < btns.size(); ++i) {
            float x = startX + (i / 2) * cellSize;
            float y = startY + (i % 2) * cellSize;

            ToolbarButton btn;
            btn.id = btns[i].first;
            btn.rect = Gdiplus::RectF(x, y, btnSize, btnSize);
            btn.isCheckable = (btn.id <= BTN_SPOTLIGHT || btn.id == BTN_HALO || btn.id == BTN_FADING || btn.id == BTN_WHITEBOARD || btn.id == BTN_BLACKBOARD);
            btn.tooltip = btns[i].second;
            m_buttons.push_back(btn);
        }

        // Color Picker Grid: 12 columns, 2 rows centered to the right of buttons
        float colorStartX = startX + ((btns.size() + 1) / 2) * cellSize + 8.0f;
        m_colorGridRect = Gdiplus::RectF(colorStartX, startY + 12.0f, 144.0f, 24.0f);

        // Custom Color dialog button next to grid
        float customStartX = colorStartX + 144.0f + 6.0f;
        m_customColorRect = Gdiplus::RectF(customStartX, startY + 12.0f, 24.0f, 24.0f);

        // Horizontal Size Slider next to custom color
        float sliderStartX = customStartX + 24.0f + 12.0f;
        m_sizeSliderRect = Gdiplus::RectF(sliderStartX, startY + 14.0f, 60.0f, 20.0f);

        m_width = static_cast<int>(sliderStartX + 60.0f + 12.0f);
    }
}

int Toolbar::HitTestButton(float x, float y) {
    for (const auto& btn : m_buttons) {
        if (btn.rect.Contains(x, y)) {
            return btn.id;
        }
    }
    return 0;
}

int Toolbar::HitTestColorGrid(float x, float y) {
    if (!m_colorGridRect.Contains(x, y)) return -1;

    float relativeX = x - m_colorGridRect.X;
    float relativeY = y - m_colorGridRect.Y;

    int col = 0, row = 0;
    if (m_orientation == ToolbarOrientation::VERTICAL) {
        col = static_cast<int>(relativeX / 12.0f);
        row = static_cast<int>(relativeY / 12.0f);
    } else {
        col = static_cast<int>(relativeX / 12.0f);
        row = static_cast<int>(relativeY / 12.0f);
    }

    col = std::max(0, std::min(col, m_orientation == ToolbarOrientation::VERTICAL ? 1 : 11));
    row = std::max(0, std::min(row, m_orientation == ToolbarOrientation::VERTICAL ? 11 : 1));

    int index = 0;
    if (m_orientation == ToolbarOrientation::VERTICAL) {
        index = row * 2 + col;
    } else {
        index = col * 2 + row;
    }

    if (index >= 0 && index < 24) {
        return index;
    }
    return -1;
}

void Toolbar::HandleClick(int btnId) {
    DrawingEngine& de = m_overlay->GetDrawingEngine();

    switch (btnId) {
        case BTN_PEN:
            de.SetTool(ToolType::PEN);
            break;
        case BTN_HIGHLIGHTER:
            de.SetTool(ToolType::HIGHLIGHTER);
            break;
        case BTN_ERASER:
            de.SetTool(ToolType::ERASER);
            break;
        case BTN_LINE:
            de.SetTool(ToolType::LINE);
            break;
        case BTN_ARROW:
            de.SetTool(ToolType::ARROW);
            break;
        case BTN_RECT:
            de.SetTool(ToolType::RECTANGLE);
            break;
        case BTN_ELLIPSE:
            de.SetTool(ToolType::ELLIPSE);
            break;
        case BTN_TEXT:
            de.SetTool(ToolType::TEXT);
            break;
        case BTN_LASER:
            de.SetTool(ToolType::LASER_POINTER);
            break;
        case BTN_SPOTLIGHT:
            de.SetTool(ToolType::SPOTLIGHT);
            break;
        case BTN_HALO:
            de.SetShowCursorHalo(!de.IsShowCursorHalo());
            break;
        case BTN_FADING:
            de.SetFadingInk(!de.IsFadingInk());
            break;
        case BTN_WHITEBOARD:
            de.SetWhiteboard(!de.IsWhiteboard());
            break;
        case BTN_BLACKBOARD:
            de.SetBlackboard(!de.IsBlackboard());
            break;
        case BTN_SCREENSHOT:
            // Temporarily hide toolbar to not capture it, take screenshot, and restore
            Show(false);
            Sleep(100);
            m_overlay->CaptureScreenToClipboard();
            Show(true);
            break;
        case BTN_UNDO:
            de.Undo();
            break;
        case BTN_CLEAR:
            de.Clear();
            break;
        case BTN_TOGGLE_ALIGN:
            SetOrientation(m_orientation == ToolbarOrientation::VERTICAL ? ToolbarOrientation::HORIZONTAL : ToolbarOrientation::VERTICAL);
            break;
        case BTN_CLOSE:
            m_overlay->SetDrawMode(false);
            break;
    }
}

LRESULT CALLBACK Toolbar::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    Toolbar* pThis = reinterpret_cast<Toolbar*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Toolbar::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NCHITTEST: {
            // Drag handle hit testing
            POINT pt = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
            ScreenToClient(hwnd, &pt);

            if (m_orientation == ToolbarOrientation::VERTICAL) {
                if (pt.y >= 0 && pt.y < 16) {
                    return HTCAPTION; // Allow drag
                }
            } else {
                if (pt.x >= 0 && pt.x < 16) {
                    return HTCAPTION; // Allow drag
                }
            }
            return HTCLIENT;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Paint(hdc, ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(static_cast<short>(LOWORD(lParam)));
            float y = static_cast<float>(static_cast<short>(HIWORD(lParam)));

            // Track hovering over buttons
            int newHoverBtn = HitTestButton(x, y);
            if (newHoverBtn != m_hoveredBtnId) {
                m_hoveredBtnId = newHoverBtn;
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            // Brush size slider dragging
            if (m_isDraggingSlider) {
                DrawingEngine& de = m_overlay->GetDrawingEngine();
                float val = 0.0f;
                if (m_orientation == ToolbarOrientation::VERTICAL) {
                    float relativeY = y - m_sizeSliderRect.Y;
                    val = relativeY / m_sizeSliderRect.Height;
                } else {
                    float relativeX = x - m_sizeSliderRect.X;
                    val = relativeX / m_sizeSliderRect.Width;
                }
                val = std::max(0.0f, std::min(val, 1.0f));
                
                // Map slider (0.0 to 1.0) to brush width (1.0 to 20.0)
                float width = 1.0f + val * 19.0f;
                de.SetBrushWidth(width);
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            // Request mouse leave notice to clear hover state
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            return 0;
        }
        case WM_MOUSELEAVE:
            if (m_hoveredBtnId != 0) {
                m_hoveredBtnId = 0;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_LBUTTONDOWN: {
            float x = static_cast<float>(static_cast<short>(LOWORD(lParam)));
            float y = static_cast<float>(static_cast<short>(HIWORD(lParam)));

            // 1. Check Color Grid click
            int colorIdx = HitTestColorGrid(x, y);
            if (colorIdx != -1) {
                m_selectedColorIndex = colorIdx;
                m_overlay->GetDrawingEngine().SetColor(m_presetColors[colorIdx]);
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            // 2. Check Custom Color picker click
            if (m_customColorRect.Contains(x, y)) {
                CHOOSECOLORW cc = {};
                static COLORREF custColors[16] = {};
                cc.lStructSize = sizeof(CHOOSECOLORW);
                cc.hwndOwner = hwnd;
                cc.lpCustColors = custColors;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                cc.rgbResult = RGB(255, 0, 0);

                if (ChooseColorW(&cc)) {
                    Gdiplus::Color custColor(255, GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult));
                    m_overlay->GetDrawingEngine().SetColor(custColor);
                    m_selectedColorIndex = -1; // Deselect presets
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }

            // 3. Check Size Slider click
            if (m_sizeSliderRect.Contains(x, y)) {
                m_isDraggingSlider = true;
                SetCapture(hwnd);
                // Call mouse move handler to set the slider value instantly
                SendMessageW(hwnd, WM_MOUSEMOVE, wParam, lParam);
                return 0;
            }

            // 4. Check Button click
            int clickedBtn = HitTestButton(x, y);
            if (clickedBtn != 0) {
                m_pressedBtnId = clickedBtn;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (m_isDraggingSlider) {
                m_isDraggingSlider = false;
                ReleaseCapture();
                return 0;
            }

            float x = static_cast<float>(static_cast<short>(LOWORD(lParam)));
            float y = static_cast<float>(static_cast<short>(HIWORD(lParam)));
            int releasedBtn = HitTestButton(x, y);

            if (m_pressedBtnId != 0 && m_pressedBtnId == releasedBtn) {
                HandleClick(m_pressedBtnId);
            }

            m_pressedBtnId = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_MOVE: {
            // Track toolbar position changes to save to config later
            m_x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
            m_y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
            return 0;
        }
        case WM_DESTROY:
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void Toolbar::Paint(HDC hdc, PAINTSTRUCT& ps) {
    (void)ps;
    
    // Set up double-buffered backbuffer to prevent flicker
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBmpMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP hOldBmp = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, hBmpMem));

    Gdiplus::Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    // 1. Paint background with modern dark charcoal glassmorphic effect
    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(220, 20, 20, 20));
    graphics.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);

    // Subtle dark border
    Gdiplus::Pen borderPen(Gdiplus::Color(80, 255, 255, 255), 1.0f);
    graphics.DrawRectangle(&borderPen, 0, 0, rc.right - 1, rc.bottom - 1);

    // 2. Draw Drag Handle (Top for vertical, Left for horizontal)
    Gdiplus::SolidBrush handleBrush(Gdiplus::Color(100, 255, 255, 255));
    if (m_orientation == ToolbarOrientation::VERTICAL) {
        graphics.FillRectangle(&handleBrush, 28.0f, 6.0f, 20.0f, 2.0f);
        graphics.FillRectangle(&handleBrush, 28.0f, 10.0f, 20.0f, 2.0f);
    } else {
        graphics.FillRectangle(&handleBrush, 6.0f, 28.0f, 2.0f, 20.0f);
        graphics.FillRectangle(&handleBrush, 10.0f, 28.0f, 2.0f, 20.0f);
    }

    DrawingEngine& de = m_overlay->GetDrawingEngine();
    ToolType activeTool = de.GetTool();

    // 3. Paint all buttons
    for (const auto& btn : m_buttons) {
        bool isHovered = (btn.id == m_hoveredBtnId);
        bool isPressed = (btn.id == m_pressedBtnId);

        // Determine if button is selected
        bool isSelected = false;
        if (btn.id == BTN_PEN && activeTool == ToolType::PEN) isSelected = true;
        else if (btn.id == BTN_HIGHLIGHTER && activeTool == ToolType::HIGHLIGHTER) isSelected = true;
        else if (btn.id == BTN_ERASER && activeTool == ToolType::ERASER) isSelected = true;
        else if (btn.id == BTN_LINE && activeTool == ToolType::LINE) isSelected = true;
        else if (btn.id == BTN_ARROW && activeTool == ToolType::ARROW) isSelected = true;
        else if (btn.id == BTN_RECT && activeTool == ToolType::RECTANGLE) isSelected = true;
        else if (btn.id == BTN_ELLIPSE && activeTool == ToolType::ELLIPSE) isSelected = true;
        else if (btn.id == BTN_TEXT && activeTool == ToolType::TEXT) isSelected = true;
        else if (btn.id == BTN_LASER && activeTool == ToolType::LASER_POINTER) isSelected = true;
        else if (btn.id == BTN_SPOTLIGHT && activeTool == ToolType::SPOTLIGHT) isSelected = true;
        else if (btn.id == BTN_HALO && de.IsShowCursorHalo()) isSelected = true;
        else if (btn.id == BTN_FADING && de.IsFadingInk()) isSelected = true;
        else if (btn.id == BTN_WHITEBOARD && de.IsWhiteboard()) isSelected = true;
        else if (btn.id == BTN_BLACKBOARD && de.IsBlackboard()) isSelected = true;

        // Draw button background based on states
        if (isPressed) {
            Gdiplus::SolidBrush prBrush(Gdiplus::Color(100, 255, 255, 255));
            graphics.FillRectangle(&prBrush, btn.rect);
        } else if (isSelected) {
            Gdiplus::SolidBrush selBrush(Gdiplus::Color(120, 100, 100, 255)); // Premium purple/violet highlights
            graphics.FillRectangle(&selBrush, btn.rect);
            // Draw border glow
            Gdiplus::Pen glowPen(Gdiplus::Color(255, 120, 120, 255), 1.5f);
            graphics.DrawRectangle(&glowPen, btn.rect);
        } else if (isHovered) {
            Gdiplus::SolidBrush hovBrush(Gdiplus::Color(50, 255, 255, 255));
            graphics.FillRectangle(&hovBrush, btn.rect);
        }

        // Subtly draw button borders
        Gdiplus::Pen btnBorder(Gdiplus::Color(40, 255, 255, 255), 1.0f);
        graphics.DrawRectangle(&btnBorder, btn.rect);

        // Draw the custom vector icon
        DrawButtonIcon(graphics, btn.id, btn.rect, isHovered, isSelected);
    }

    // 4. Paint 24-color preset grid
    Gdiplus::Pen gridBorder(Gdiplus::Color(80, 255, 255, 255), 1.0f);
    graphics.DrawRectangle(&gridBorder, m_colorGridRect);

    int cols = (m_orientation == ToolbarOrientation::VERTICAL) ? 2 : 12;
    int rows = (m_orientation == ToolbarOrientation::VERTICAL) ? 12 : 2;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int index = (m_orientation == ToolbarOrientation::VERTICAL) ? (r * 2 + c) : (c * 2 + r);
            if (index >= 0 && index < 24) {
                float cellX = m_colorGridRect.X + c * 12.0f + 1.0f;
                float cellY = m_colorGridRect.Y + r * 12.0f + 1.0f;

                Gdiplus::SolidBrush cBrush(m_presetColors[index]);
                graphics.FillRectangle(&cBrush, cellX, cellY, 10.0f, 10.0f);

                // Highlight selected color index
                if (index == m_selectedColorIndex) {
                    Gdiplus::Pen selectPen(Gdiplus::Color(255, 255, 255, 255), 1.0f);
                    graphics.DrawRectangle(&selectPen, cellX, cellY, 10.0f, 10.0f);
                }
            }
        }
    }

    // 5. Paint Custom Color Dialog indicator button
    Gdiplus::Color curColor = de.GetColor();
    Gdiplus::SolidBrush activeColorBrush(curColor);
    graphics.FillRectangle(&activeColorBrush, m_customColorRect);
    graphics.DrawRectangle(&gridBorder, m_customColorRect);
    
    // Draw small custom icon "+" inside custom color picker to identify it
    Gdiplus::Pen plusPen(Gdiplus::Color(255, 255, 255, 255), 2.0f);
    if (curColor.GetRed() > 200 && curColor.GetGreen() > 200 && curColor.GetBlue() > 200) {
        plusPen.SetColor(Gdiplus::Color(255, 0, 0, 0)); // black plus on bright colors
    }
    float cx = m_customColorRect.X + m_customColorRect.Width / 2.0f;
    float cy = m_customColorRect.Y + m_customColorRect.Height / 2.0f;
    graphics.DrawLine(&plusPen, cx - 5.0f, cy, cx + 5.0f, cy);
    graphics.DrawLine(&plusPen, cx, cy - 5.0f, cx, cy + 5.0f);

    // 6. Paint Brush Size Slider
    graphics.DrawRectangle(&gridBorder, m_sizeSliderRect);
    float widthVal = de.GetBrushWidth();
    float pct = (widthVal - 1.0f) / 19.0f; // Slider percentage (0.0 to 1.0)

    Gdiplus::SolidBrush sliderFill(Gdiplus::Color(100, 255, 255, 255));
    if (m_orientation == ToolbarOrientation::VERTICAL) {
        float fillHeight = m_sizeSliderRect.Height * pct;
        // Draw track fill starting from the top representing thickness increase
        graphics.FillRectangle(&sliderFill, m_sizeSliderRect.X, m_sizeSliderRect.Y, m_sizeSliderRect.Width, fillHeight);
        
        // Draw slider handle line
        Gdiplus::Pen handlePen(Gdiplus::Color(255, 255, 0, 0), 2.0f);
        graphics.DrawLine(&handlePen, m_sizeSliderRect.X, m_sizeSliderRect.Y + fillHeight, m_sizeSliderRect.X + m_sizeSliderRect.Width, m_sizeSliderRect.Y + fillHeight);
    } else {
        float fillWidth = m_sizeSliderRect.Width * pct;
        graphics.FillRectangle(&sliderFill, m_sizeSliderRect.X, m_sizeSliderRect.Y, fillWidth, m_sizeSliderRect.Height);

        Gdiplus::Pen handlePen(Gdiplus::Color(255, 255, 0, 0), 2.0f);
        graphics.DrawLine(&handlePen, m_sizeSliderRect.X + fillWidth, m_sizeSliderRect.Y, m_sizeSliderRect.X + fillWidth, m_sizeSliderRect.Y + m_sizeSliderRect.Height);
    }

    // Double buffer copy to screen
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmpMem);
    DeleteDC(hdcMem);
}

void Toolbar::DrawButtonIcon(Gdiplus::Graphics& graphics, int btnId, const Gdiplus::RectF& rect, bool isHovered, bool isSelected) {
    (void)isHovered;
    // Set up standard drawing pen/brush
    Gdiplus::Color iconColor = isSelected ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 220, 220, 220);
    Gdiplus::Pen pen(iconColor, 1.5f);
    Gdiplus::SolidBrush brush(iconColor);

    float cx = rect.X + rect.Width / 2.0f;
    float cy = rect.Y + rect.Height / 2.0f;

    switch (btnId) {
        case BTN_PEN: {
            // Draw Pencil shape diagonal
            graphics.DrawLine(&pen, cx - 6.0f, cy + 6.0f, cx + 6.0f, cy - 6.0f);
            graphics.DrawLine(&pen, cx - 5.0f, cy + 7.0f, cx + 7.0f, cy - 5.0f);
            
            // Draw tip
            Gdiplus::PointF tip[3] = {
                Gdiplus::PointF(cx - 8.0f, cy + 8.0f),
                Gdiplus::PointF(cx - 7.0f, cy + 5.0f),
                Gdiplus::PointF(cx - 5.0f, cy + 7.0f)
            };
            graphics.FillPolygon(&brush, tip, 3);
            break;
        }
        case BTN_HIGHLIGHTER: {
            // Draw thick highlighter tip diagonal
            Gdiplus::Pen thickPen(iconColor, 3.5f);
            graphics.DrawLine(&thickPen, cx - 5.0f, cy + 5.0f, cx + 5.0f, cy - 5.0f);
            // Draw cap
            Gdiplus::SolidBrush capBrush(Gdiplus::Color(100, 255, 255, 0)); // yellow highlighter cap overlay
            graphics.FillRectangle(&capBrush, cx + 1.0f, cy - 7.0f, 6.0f, 6.0f);
            break;
        }
        case BTN_ERASER: {
            // Draw diagonal eraser block
            Gdiplus::GraphicsState state = graphics.Save();
            graphics.TranslateTransform(cx, cy);
            graphics.RotateTransform(-30.0f);
            
            // Draw pink eraser base and white body
            Gdiplus::SolidBrush pinkBrush(Gdiplus::Color(255, 255, 120, 150));
            Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 240, 240, 240));
            graphics.FillRectangle(&pinkBrush, -8.0f, -4.0f, 8.0f, 8.0f);
            graphics.FillRectangle(&whiteBrush, 0.0f, -4.0f, 8.0f, 8.0f);
            
            graphics.Restore(state);
            break;
        }
        case BTN_LINE: {
            // Draw straight diagonal line
            graphics.DrawLine(&pen, cx - 7.0f, cy + 7.0f, cx + 7.0f, cy - 7.0f);
            break;
        }
        case BTN_ARROW: {
            // Draw line with arrowhead pointing top-right
            Gdiplus::PointF start(cx - 7.0f, cy + 7.0f);
            Gdiplus::PointF end(cx + 5.0f, cy - 5.0f);
            graphics.DrawLine(&pen, start, end);
            
            // Draw arrowhead polygon
            Gdiplus::PointF tip[3] = {
                Gdiplus::PointF(cx + 8.0f, cy - 8.0f),
                Gdiplus::PointF(cx + 1.0f, cy - 6.0f),
                Gdiplus::PointF(cx + 6.0f, cy - 1.0f)
            };
            graphics.FillPolygon(&brush, tip, 3);
            break;
        }
        case BTN_RECT: {
            // Draw rectangle outline
            graphics.DrawRectangle(&pen, cx - 7.0f, cy - 6.0f, 14.0f, 12.0f);
            break;
        }
        case BTN_ELLIPSE: {
            // Draw ellipse outline
            graphics.DrawEllipse(&pen, cx - 8.0f, cy - 6.0f, 16.0f, 12.0f);
            break;
        }
        case BTN_TEXT: {
            // Draw big A letter
            Gdiplus::FontFamily ff(L"Arial");
            Gdiplus::Font f(&ff, 12.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::PointF origin(cx - 5.0f, cy - 6.0f);
            graphics.DrawString(L"T", -1, &f, origin, &brush);
            break;
        }
        case BTN_LASER: {
            // Red glowing laser pointer icon
            Gdiplus::SolidBrush laserBrush(Gdiplus::Color(255, 255, 0, 0));
            graphics.FillEllipse(&laserBrush, cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            Gdiplus::SolidBrush laserCore(Gdiplus::Color(255, 255, 255, 255));
            graphics.FillEllipse(&laserCore, cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
            break;
        }
        case BTN_SPOTLIGHT: {
            // Spotlight circle cone
            Gdiplus::SolidBrush darkCon(Gdiplus::Color(180, 50, 50, 50));
            graphics.FillRectangle(&darkCon, cx - 9.0f, cy - 9.0f, 18.0f, 18.0f);
            
            // Spotlight center hole bright circle
            Gdiplus::SolidBrush brightHole(Gdiplus::Color(255, 255, 255, 255));
            graphics.FillEllipse(&brightHole, cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
            break;
        }
        case BTN_HALO: {
            // Cursor pointer with yellow halo around it
            Gdiplus::SolidBrush haloBrush(Gdiplus::Color(100, 255, 255, 0));
            graphics.FillEllipse(&haloBrush, cx - 6.0f, cy - 6.0f, 12.0f, 12.0f);
            
            // Draw classic mouse pointer arrowhead
            Gdiplus::PointF arrow[3] = {
                Gdiplus::PointF(cx - 1.0f, cy - 1.0f),
                Gdiplus::PointF(cx + 4.0f, cy + 4.0f),
                Gdiplus::PointF(cx + 1.0f, cy + 5.0f)
            };
            graphics.FillPolygon(&brush, arrow, 3);
            break;
        }
        case BTN_FADING: {
            // Clock/Drop representing fading ink
            graphics.DrawEllipse(&pen, cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
            // Clock hands
            graphics.DrawLine(&pen, cx, cy, cx, cy - 4.0f);
            graphics.DrawLine(&pen, cx, cy, cx + 3.0f, cy);
            break;
        }
        case BTN_WHITEBOARD: {
            // Solid white square representing whiteboard
            Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
            graphics.FillRectangle(&whiteBrush, cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
            graphics.DrawRectangle(&pen, cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
            break;
        }
        case BTN_BLACKBOARD: {
            // Solid black square representing blackboard
            Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 1, 1, 1));
            graphics.FillRectangle(&blackBrush, cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
            graphics.DrawRectangle(&pen, cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
            break;
        }
        case BTN_SCREENSHOT: {
            // Camera icon
            graphics.DrawRectangle(&pen, cx - 8.0f, cy - 5.0f, 16.0f, 11.0f);
            graphics.DrawEllipse(&pen, cx - 4.0f, cy - 2.0f, 8.0f, 6.0f);
            // Flash flash bulb
            graphics.FillRectangle(&brush, cx - 5.0f, cy - 7.0f, 3.0f, 2.0f);
            break;
        }
        case BTN_UNDO: {
            // Left turning curve arrow
            graphics.DrawArc(&pen, cx - 6.0f, cy - 6.0f, 12.0f, 12.0f, 90.0f, 180.0f);
            // Tip pointing left
            Gdiplus::PointF tip[3] = {
                Gdiplus::PointF(cx - 6.0f, cy),
                Gdiplus::PointF(cx - 3.0f, cy - 3.0f),
                Gdiplus::PointF(cx - 3.0f, cy + 3.0f)
            };
            graphics.FillPolygon(&brush, tip, 3);
            break;
        }
        case BTN_CLEAR: {
            // Trash can icon
            graphics.DrawRectangle(&pen, cx - 6.0f, cy - 3.0f, 12.0f, 10.0f);
            // Lid
            graphics.DrawLine(&pen, cx - 8.0f, cy - 5.0f, cx + 8.0f, cy - 5.0f);
            graphics.DrawLine(&pen, cx - 3.0f, cy - 5.0f, cx - 3.0f, cy - 7.0f);
            graphics.DrawLine(&pen, cx + 3.0f, cy - 5.0f, cx + 3.0f, cy - 7.0f);
            // Center slats
            graphics.DrawLine(&pen, cx, cy - 1.0f, cx, cy + 5.0f);
            graphics.DrawLine(&pen, cx - 3.0f, cy - 1.0f, cx - 3.0f, cy + 5.0f);
            graphics.DrawLine(&pen, cx + 3.0f, cy - 1.0f, cx + 3.0f, cy + 5.0f);
            break;
        }
        case BTN_TOGGLE_ALIGN: {
            // Two interlocking rectangles horizontal and vertical
            graphics.DrawRectangle(&pen, cx - 9.0f, cy - 4.0f, 18.0f, 8.0f);
            graphics.DrawRectangle(&pen, cx - 4.0f, cy - 9.0f, 8.0f, 18.0f);
            break;
        }
        case BTN_CLOSE: {
            // Clean exit cross mark icon
            Gdiplus::Pen thickPen(iconColor, 2.0f);
            graphics.DrawLine(&thickPen, cx - 6.0f, cy - 6.0f, cx + 6.0f, cy + 6.0f);
            graphics.DrawLine(&thickPen, cx - 6.0f, cy + 6.0f, cx + 6.0f, cy - 6.0f);
            break;
        }
    }
}
