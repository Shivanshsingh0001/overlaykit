#include "pinned_panel.h"
#include "platform.h"
#include <algorithm>
#include <fstream>
#include <shellapi.h>

PinnedPanel::PinnedPanel()
    : m_hwnd(nullptr), m_ownerHwnd(nullptr), m_id(0),
      m_scrollY(0), m_maxScrollY(0),
      m_isImage(false), m_image(nullptr) {}

PinnedPanel::~PinnedPanel() {
    ClearContent();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void PinnedPanel::ClearContent() {
    if (m_image) {
        delete m_image;
        m_image = nullptr;
    }
    m_text.clear();
    m_filePath.clear();
    m_isImage = false;
    m_scrollY = 0;
    m_maxScrollY = 0;
}

bool PinnedPanel::Create(HWND ownerHwnd, HINSTANCE hInstance, int id, int x, int y, int w, int h) {
    m_ownerHwnd = ownerHwnd;
    m_id = id;

    // Register class for pinned panel if not done
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = PinnedPanel::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"OverlayKit_PinnedPanel_Class";

    if (!RegisterClassExW(&wc)) {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    wchar_t title[64];
    swprintf_s(title, L"OverlayKit Pinned Panel #%d", id);

    // Create a resizable popup window with a caption and close box
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"OverlayKit_PinnedPanel_Class",
        title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE,
        x, y, w, h,
        ownerHwnd, nullptr, hInstance, this
    );

    if (!m_hwnd) return false;

    // Exclude from screen recording and screenshots
    Platform::SetCaptureExclusion(m_hwnd, true);

    // Enable drag and drop
    DragAcceptFiles(m_hwnd, TRUE);

    return true;
}

void PinnedPanel::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(m_hwnd);
    }
}

bool PinnedPanel::LoadFile(const std::wstring& filePath) {
    ClearContent();
    m_filePath = filePath;

    // Resolve extension
    size_t dot = filePath.find_last_of(L'.');
    std::wstring ext = L"";
    if (dot != std::wstring::npos) {
        ext = filePath.substr(dot);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    }

    // Attempt GDI+ image load first if extension matches
    if (ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".gif" || ext == L".bmp" || ext == L".tiff" || ext == L".tif") {
        m_image = Gdiplus::Image::FromFile(filePath.c_str());
        if (m_image && m_image->GetLastStatus() == Gdiplus::Ok) {
            m_isImage = true;
            return true;
        }
        if (m_image) {
            delete m_image;
            m_image = nullptr;
        }
    }

    // Otherwise, try to load as text
    std::ifstream in(filePath.c_str(), std::ios::binary);
    if (!in.is_open()) return false;

    // Read up to 256KB to avoid massive hangs
    std::string bytes;
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer)) && bytes.size() < 262144) {
        bytes.append(buffer, in.gcount());
    }
    bytes.append(buffer, in.gcount());
    in.close();

    // Prevent loading raw binary as text
    size_t checkLen = (std::min)(bytes.size(), static_cast<size_t>(1024));
    for (size_t i = 0; i < checkLen; ++i) {
        if (bytes[i] == '\0') {
            return false;
        }
    }

    // Convert from UTF-8 to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, bytes.c_str(), static_cast<int>(bytes.size()), nullptr, 0);
    if (wlen > 0) {
        m_text.resize(wlen);
        MultiByteToWideChar(CP_UTF8, 0, bytes.c_str(), static_cast<int>(bytes.size()), &m_text[0], wlen);
    } else {
        // Fallback to ANSI
        wlen = MultiByteToWideChar(CP_ACP, 0, bytes.c_str(), static_cast<int>(bytes.size()), nullptr, 0);
        if (wlen > 0) {
            m_text.resize(wlen);
            MultiByteToWideChar(CP_ACP, 0, bytes.c_str(), static_cast<int>(bytes.size()), &m_text[0], wlen);
        } else {
            return false;
        }
    }

    m_isImage = false;
    return true;
}

LRESULT CALLBACK PinnedPanel::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    PinnedPanel* pThis = reinterpret_cast<PinnedPanel*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT PinnedPanel::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        m_hwnd = hwnd;
    }

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            if (width <= 0 || height <= 0) {
                EndPaint(hwnd, &ps);
                return 0;
            }

            // Create double buffer to eliminate flicker
            HDC hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP hOldBmp = reinterpret_cast<HBITMAP>(SelectObject(hMemDC, hMemBmp));

            {
                Gdiplus::Graphics graphics(hMemDC);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

                // Paint dark workspace theme background
                Gdiplus::SolidBrush bgBrush(Gdiplus::Color(30, 30, 30));
                graphics.FillRectangle(&bgBrush, 0, 0, width, height);

                if (m_isImage && m_image) {
                    float imgW = static_cast<float>(m_image->GetWidth());
                    float imgH = static_cast<float>(m_image->GetHeight());

                    if (imgW > 0 && imgH > 0) {
                        float scale = (std::min)(static_cast<float>(width) / imgW, static_cast<float>(height) / imgH);
                        float drawW = imgW * scale;
                        float drawH = imgH * scale;
                        float drawX = (static_cast<float>(width) - drawW) / 2.0f;
                        float drawY = (static_cast<float>(height) - drawH) / 2.0f;

                        graphics.DrawImage(m_image, drawX, drawY, drawW, drawH);
                    }
                } else if (!m_text.empty()) {
                    Gdiplus::Font font(L"Segoe UI", 10.0f, Gdiplus::FontStyleRegular);
                    Gdiplus::SolidBrush textBrush(Gdiplus::Color(240, 240, 240));

                    Gdiplus::StringFormat format;
                    format.SetFormatFlags(0);

                    // Scrolled layout rect
                    Gdiplus::RectF layoutRect(
                        12.0f,
                        12.0f - static_cast<float>(m_scrollY),
                        static_cast<float>(width) - 24.0f,
                        100000.0f
                    );

                    // Measure text height to clamp scrolling boundaries
                    Gdiplus::RectF boundingBox;
                    graphics.MeasureString(
                        m_text.c_str(), -1, &font,
                        Gdiplus::RectF(12.0f, 12.0f, static_cast<float>(width) - 24.0f, 100000.0f),
                        &format, &boundingBox
                    );
                    m_maxScrollY = static_cast<int>((std::max)(0.0f, boundingBox.Height - static_cast<float>(height) + 24.0f));

                    // Keep scrolled rendering clipped inside the margins
                    Gdiplus::RectF clipRect(10.0f, 10.0f, static_cast<float>(width) - 20.0f, static_cast<float>(height) - 20.0f);
                    graphics.SetClip(clipRect);

                    graphics.DrawString(m_text.c_str(), -1, &font, layoutRect, &format, &textBrush);
                    graphics.ResetClip();
                } else {
                    // Draw Drag & Drop instructions
                    Gdiplus::Font font(L"Segoe UI", 10.0f, Gdiplus::FontStyleItalic);
                    Gdiplus::SolidBrush textBrush(Gdiplus::Color(150, 150, 150));

                    Gdiplus::StringFormat format;
                    format.SetAlignment(Gdiplus::StringAlignmentCenter);
                    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

                    Gdiplus::RectF layoutRect(10.0f, 10.0f, static_cast<float>(width) - 20.0f, static_cast<float>(height) - 20.0f);
                    graphics.DrawString(L"Drag & drop a text or image file here", -1, &font, layoutRect, &format, &textBrush);

                    // Draw a stylized dashed drop zone border
                    Gdiplus::Pen borderPen(Gdiplus::Color(80, 80, 80), 1.0f);
                    borderPen.SetDashStyle(Gdiplus::DashStyleDash);
                    graphics.DrawRectangle(&borderPen, 15, 15, width - 30, height - 30);
                }

                // Fine dark border outline
                Gdiplus::Pen outlinePen(Gdiplus::Color(63, 63, 70), 1.0f);
                graphics.DrawRectangle(&outlinePen, 0, 0, width - 1, height - 1);
            }

            // Blit buffer to window
            BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);

            SelectObject(hMemDC, hOldBmp);
            DeleteObject(hMemBmp);
            DeleteDC(hMemDC);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            if (!m_text.empty()) {
                short delta = GET_WHEEL_DELTA_WPARAM(wParam);
                m_scrollY -= (delta / 120) * 20; // Scroll 20px per detent
                if (m_scrollY < 0) m_scrollY = 0;
                if (m_scrollY > m_maxScrollY) m_scrollY = m_maxScrollY;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }

        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;

        case WM_ERASEBKGND:
            return 1; // Bypass background repaint to avoid flicker

        case WM_DROPFILES: {
            HDROP hDrop = reinterpret_cast<HDROP>(wParam);
            wchar_t filePath[MAX_PATH];
            if (DragQueryFileW(hDrop, 0, filePath, MAX_PATH)) {
                LoadFile(filePath);
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            DragFinish(hDrop);
            return 0;
        }

        case WM_DESTROY:
            // Notify owner window that this panel is closing (if required)
            // Send message or post to parent
            PostMessageW(m_ownerHwnd, WM_USER + 300, m_id, 0);
            break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
