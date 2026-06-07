#include "drawing_engine.h"
#include <algorithm>
#include <cmath>

DrawingEngine::DrawingEngine()
    : m_hwnd(nullptr), m_width(0), m_height(0), m_isDrawing(false),
      m_backgroundCanvas(nullptr), m_permanentCanvas(nullptr),
      m_currentTool(ToolType::PEN),
      m_currentColor(Gdiplus::Color(255, 255, 0, 0)), // Default Red
      m_currentWidth(4.0f),
      m_whiteboardActive(false), m_blackboardActive(false),
      m_fadingInkEnabled(false),
      m_showCursorHalo(false),
      m_cursorPos(Gdiplus::PointF(0.0f, 0.0f)) {
    m_activeStroke.points.reserve(100);
}

DrawingEngine::~DrawingEngine() {
    Shutdown();
}

bool DrawingEngine::Initialize(HWND hwnd, int width, int height) {
    m_hwnd = hwnd;
    m_width = width;
    m_height = height;

    // Create the background and permanent bitmaps
    m_backgroundCanvas = new Gdiplus::Bitmap(m_width, m_height, PixelFormat32bppARGB);
    m_permanentCanvas = new Gdiplus::Bitmap(m_width, m_height, PixelFormat32bppARGB);
    
    // Clear everything
    Clear();

    return m_backgroundCanvas != nullptr && m_permanentCanvas != nullptr;
}

void DrawingEngine::Shutdown() {
    if (m_backgroundCanvas) {
        delete m_backgroundCanvas;
        m_backgroundCanvas = nullptr;
    }
    if (m_permanentCanvas) {
        delete m_permanentCanvas;
        m_permanentCanvas = nullptr;
    }
    m_strokeHistory.clear();
    m_fadingStrokes.clear();
}

void DrawingEngine::Resize(int width, int height) {
    if (width == m_width && height == m_height) return;

    int oldWidth = m_width;
    int oldHeight = m_height;

    m_width = width;
    m_height = height;

    Gdiplus::Bitmap* oldBackground = m_backgroundCanvas;
    Gdiplus::Bitmap* oldPermanent = m_permanentCanvas;
    
    m_backgroundCanvas = new Gdiplus::Bitmap(m_width, m_height, PixelFormat32bppARGB);
    m_permanentCanvas = new Gdiplus::Bitmap(m_width, m_height, PixelFormat32bppARGB);

    // Clear and copy old permanent contents
    {
        Gdiplus::Graphics g(m_permanentCanvas);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
        if (oldPermanent) {
            // Draw old content to scale if display changed
            g.DrawImage(oldPermanent, 0, 0, (std::min)(oldWidth, m_width), (std::min)(oldHeight, m_height));
        }
    }

    // Redraw all onto the new background canvas
    RedrawBackgroundCanvas();

    if (oldBackground) delete oldBackground;
    if (oldPermanent) delete oldPermanent;
}

void DrawingEngine::StartStroke(float x, float y) {
    if (m_isDrawing) return;

    // Laser pointer and Spotlight don't draw persistent strokes
    if (m_currentTool == ToolType::LASER_POINTER || m_currentTool == ToolType::SPOTLIGHT) {
        m_cursorPos = Gdiplus::PointF(x, y);
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }

    m_isDrawing = true;
    m_activeStroke.points.clear();
    m_activeStroke.points.push_back(Gdiplus::PointF(x, y));
    m_activeStroke.color = m_currentColor;
    m_activeStroke.width = m_currentWidth;
    m_activeStroke.isHighlighter = (m_currentTool == ToolType::HIGHLIGHTER);
    m_activeStroke.isEraser = (m_currentTool == ToolType::ERASER);
    
    m_activeStroke.isShape = (m_currentTool == ToolType::LINE || 
                              m_currentTool == ToolType::ARROW || 
                              m_currentTool == ToolType::RECTANGLE || 
                              m_currentTool == ToolType::ELLIPSE);
    m_activeStroke.shapeType = m_currentTool;

    m_activeStroke.isFading = m_fadingInkEnabled && (m_currentTool == ToolType::PEN || m_currentTool == ToolType::HIGHLIGHTER);
    m_activeStroke.opacity = 1.0f;
    m_activeStroke.creationTime = GetTickCount64();

    // For shapes, add a temporary second point immediately
    if (m_activeStroke.isShape) {
        m_activeStroke.points.push_back(Gdiplus::PointF(x, y));
    }
}

void DrawingEngine::AddPointToStroke(float x, float y) {
    m_cursorPos = Gdiplus::PointF(x, y);

    if (m_currentTool == ToolType::LASER_POINTER || m_currentTool == ToolType::SPOTLIGHT) {
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }

    if (!m_isDrawing) return;

    if (m_activeStroke.isShape) {
        // Shapes track start (points[0]) and end (points[1])
        if (m_activeStroke.points.size() < 2) {
            m_activeStroke.points.push_back(Gdiplus::PointF(x, y));
        } else {
            m_activeStroke.points[1] = Gdiplus::PointF(x, y);
        }
    } else {
        // Freehand tools append points
        if (!m_activeStroke.points.empty()) {
            const auto& lastPoint = m_activeStroke.points.back();
            if (std::abs(lastPoint.X - x) < 0.5f && std::abs(lastPoint.Y - y) < 0.5f) {
                return;
            }
        }
        m_activeStroke.points.push_back(Gdiplus::PointF(x, y));
    }

    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawingEngine::EndStroke() {
    if (!m_isDrawing) {
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }
    m_isDrawing = false;

    if (!m_activeStroke.points.empty()) {
        if (m_activeStroke.isFading) {
            // Fading ink goes to the fading list, not history
            m_activeStroke.creationTime = GetTickCount64();
            m_fadingStrokes.push_back(m_activeStroke);
        } else {
            m_strokeHistory.push_back(m_activeStroke);

            // Cap the history, flattening oldest onto permanent canvas
            if (m_strokeHistory.size() > MAX_UNDO_STEPS) {
                if (m_permanentCanvas) {
                    Gdiplus::Graphics g(m_permanentCanvas);
                    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                    DrawStrokeToGraphics(g, m_strokeHistory.front());
                }
                m_strokeHistory.erase(m_strokeHistory.begin());
            }

            // Immediately write new stroke to background canvas to optimize
            if (m_backgroundCanvas) {
                Gdiplus::Graphics g(m_backgroundCanvas);
                g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                DrawStrokeToGraphics(g, m_activeStroke);
            }
        }
    }

    m_activeStroke.points.clear();

    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawingEngine::Undo() {
    if (m_strokeHistory.empty()) return;

    m_strokeHistory.pop_back();
    RedrawBackgroundCanvas();

    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawingEngine::Clear() {
    m_strokeHistory.clear();
    m_fadingStrokes.clear();
    m_activeStroke.points.clear();
    m_isDrawing = false;

    if (m_permanentCanvas) {
        Gdiplus::Graphics g(m_permanentCanvas);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
    }

    RedrawBackgroundCanvas();

    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawingEngine::UpdateFadingStrokes() {
    if (m_fadingStrokes.empty()) return;

    ULONGLONG now = GetTickCount64();
    bool changed = false;

    for (auto it = m_fadingStrokes.begin(); it != m_fadingStrokes.end();) {
        ULONGLONG elapsed = now - it->creationTime;
        const ULONGLONG fadeDuration = 3000; // 3 seconds fade duration

        if (elapsed >= fadeDuration) {
            it = m_fadingStrokes.erase(it);
            changed = true;
        } else {
            it->opacity = 1.0f - (static_cast<float>(elapsed) / static_cast<float>(fadeDuration));
            it = ++it;
            changed = true;
        }
    }

    if (changed && m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawingEngine::CommitText(const std::wstring& text, float x, float y, const std::wstring& fontName, float fontSize) {
    if (text.empty()) return;

    Stroke textStroke;
    textStroke.points.push_back(Gdiplus::PointF(x, y));
    textStroke.color = m_currentColor;
    textStroke.width = 1.0f;
    textStroke.isHighlighter = false;
    textStroke.isEraser = false;
    textStroke.isShape = true;
    textStroke.shapeType = ToolType::TEXT;
    textStroke.textContent = text;
    textStroke.fontName = fontName;
    textStroke.fontSize = fontSize;
    textStroke.isFading = false;
    textStroke.opacity = 1.0f;
    textStroke.creationTime = GetTickCount64();

    m_strokeHistory.push_back(textStroke);

    if (m_strokeHistory.size() > MAX_UNDO_STEPS) {
        if (m_permanentCanvas) {
            Gdiplus::Graphics g(m_permanentCanvas);
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            DrawStrokeToGraphics(g, m_strokeHistory.front());
        }
        m_strokeHistory.erase(m_strokeHistory.begin());
    }

    // Immediately render onto background canvas
    if (m_backgroundCanvas) {
        Gdiplus::Graphics g(m_backgroundCanvas);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        DrawStrokeToGraphics(g, textStroke);
    }

    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawingEngine::Draw(Gdiplus::Graphics& graphics) {
    // 1. Draw the pre-rendered background canvas (which holds whiteboard/blackboard background + permanent + history)
    if (m_backgroundCanvas) {
        graphics.DrawImage(m_backgroundCanvas, 0, 0);
    }

    // 2. Draw fading strokes on top (in real time)
    for (const auto& stroke : m_fadingStrokes) {
        DrawStrokeToGraphics(graphics, stroke);
    }

    // 3. Draw the active stroke in progress
    if (m_isDrawing) {
        DrawStrokeToGraphics(graphics, m_activeStroke);
    }

    // 4. Draw Spotlight effect
    if (m_currentTool == ToolType::SPOTLIGHT) {
        Gdiplus::GraphicsPath path(Gdiplus::FillModeAlternate);
        path.AddRectangle(Gdiplus::RectF(0, 0, static_cast<float>(m_width), static_cast<float>(m_height)));
        
        float radius = 150.0f; // Spotlight radius
        path.AddEllipse(m_cursorPos.X - radius, m_cursorPos.Y - radius, radius * 2.0f, radius * 2.0f);
        
        Gdiplus::SolidBrush dimBrush(Gdiplus::Color(180, 0, 0, 0)); // 70% dimming overlay
        graphics.FillPath(&dimBrush, &path);
    }

    // 5. Draw Laser Pointer
    if (m_currentTool == ToolType::LASER_POINTER) {
        float r = 8.0f;
        Gdiplus::SolidBrush laserBrush(Gdiplus::Color(255, 255, 0, 0));
        graphics.FillEllipse(&laserBrush, m_cursorPos.X - r, m_cursorPos.Y - r, r * 2.0f, r * 2.0f);
        
        float innerR = 3.0f;
        Gdiplus::SolidBrush coreBrush(Gdiplus::Color(255, 255, 255, 255));
        graphics.FillEllipse(&coreBrush, m_cursorPos.X - innerR, m_cursorPos.Y - innerR, innerR * 2.0f, innerR * 2.0f);
    }

    // 6. Draw Highlighted Cursor (Halo)
    if (m_showCursorHalo && m_currentTool != ToolType::LASER_POINTER && m_currentTool != ToolType::SPOTLIGHT) {
        float radius = 25.0f;
        Gdiplus::SolidBrush haloBrush(Gdiplus::Color(80, 255, 255, 0)); // Semi-transparent yellow
        graphics.FillEllipse(&haloBrush, m_cursorPos.X - radius, m_cursorPos.Y - radius, radius * 2.0f, radius * 2.0f);
    }
}

void DrawingEngine::SetTool(ToolType tool) {
    m_currentTool = tool;
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawingEngine::SetColor(Gdiplus::Color color) {
    m_currentColor = color;
}

void DrawingEngine::SetBrushWidth(float width) {
    m_currentWidth = width;
}

void DrawingEngine::SetWhiteboard(bool enable) {
    m_whiteboardActive = enable;
    if (m_whiteboardActive) m_blackboardActive = false;
    RedrawBackgroundCanvas();
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawingEngine::SetBlackboard(bool enable) {
    m_blackboardActive = enable;
    if (m_blackboardActive) m_whiteboardActive = false;
    RedrawBackgroundCanvas();
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawingEngine::SetFadingInk(bool enable) {
    m_fadingInkEnabled = enable;
}

void DrawingEngine::UpdateCursorPosition(float x, float y) {
    m_cursorPos = Gdiplus::PointF(x, y);
    if (m_hwnd && (m_showCursorHalo || m_currentTool == ToolType::LASER_POINTER || m_currentTool == ToolType::SPOTLIGHT)) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawingEngine::SetShowCursorHalo(bool enable) {
    m_showCursorHalo = enable;
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawingEngine::RedrawBackgroundCanvas() {
    if (!m_backgroundCanvas || !m_permanentCanvas) return;

    Gdiplus::Graphics g(m_backgroundCanvas);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    // 1. Draw whiteboard, blackboard, or transparent background
    if (m_whiteboardActive) {
        g.Clear(Gdiplus::Color(255, 255, 255, 255));
    } else if (m_blackboardActive) {
        g.Clear(Gdiplus::Color(255, 0, 0, 0));
    } else {
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
    }

    // 2. Draw the permanent strokes first
    g.DrawImage(m_permanentCanvas, 0, 0);

    // 3. Draw the undo history strokes
    for (const auto& stroke : m_strokeHistory) {
        DrawStrokeToGraphics(g, stroke);
    }
}

void DrawingEngine::DrawStrokeToGraphics(Gdiplus::Graphics& graphics, const Stroke& stroke) {
    if (stroke.points.empty()) return;

    Gdiplus::Color drawColor = stroke.color;
    
    // Prevent pure black transparency key collision
    if (drawColor.GetRed() == 0 && drawColor.GetGreen() == 0 && drawColor.GetBlue() == 0) {
        drawColor = Gdiplus::Color(drawColor.GetAlpha(), 1, 1, 1);
    }

    if (stroke.isHighlighter) {
        // Highlighters use semi-transparency (alpha = 100)
        drawColor = Gdiplus::Color(100, drawColor.GetRed(), drawColor.GetGreen(), drawColor.GetBlue());
    }

    // Adjust drawing opacity for fading strokes
    if (stroke.isFading) {
        float combinedAlpha = static_cast<float>(drawColor.GetAlpha()) * stroke.opacity;
        drawColor = Gdiplus::Color(static_cast<BYTE>(combinedAlpha), drawColor.GetRed(), drawColor.GetGreen(), drawColor.GetBlue());
    }

    // Eraser rendering via CompositingModeSourceCopy
    if (stroke.isEraser) {
        Gdiplus::CompositingMode oldMode = graphics.GetCompositingMode();
        graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);

        Gdiplus::Pen eraserPen(Gdiplus::Color(0, 0, 0, 0), stroke.width);
        eraserPen.SetStartCap(Gdiplus::LineCapRound);
        eraserPen.SetEndCap(Gdiplus::LineCapRound);
        eraserPen.SetLineJoin(Gdiplus::LineJoinRound);

        if (stroke.points.size() == 1) {
            Gdiplus::SolidBrush brush(Gdiplus::Color(0, 0, 0, 0));
            float r = stroke.width / 2.0f;
            graphics.FillEllipse(&brush, stroke.points[0].X - r, stroke.points[0].Y - r, stroke.width, stroke.width);
        } else if (stroke.points.size() == 2) {
            graphics.DrawLine(&eraserPen, stroke.points[0], stroke.points[1]);
        } else {
            graphics.DrawCurve(&eraserPen, stroke.points.data(), static_cast<INT>(stroke.points.size()));
        }

        graphics.SetCompositingMode(oldMode);
        return;
    }

    // Text tool rendering
    if (stroke.isShape && stroke.shapeType == ToolType::TEXT) {
        Gdiplus::FontFamily fontFamily(stroke.fontName.c_str());
        Gdiplus::FontFamily* pFontFamily = &fontFamily;
        if (fontFamily.GetLastStatus() != Gdiplus::Ok) {
            static Gdiplus::FontFamily fallbackFamily(L"Arial");
            pFontFamily = &fallbackFamily;
        }

        Gdiplus::Font font(pFontFamily, stroke.fontSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        Gdiplus::SolidBrush brush(drawColor);
        graphics.DrawString(stroke.textContent.c_str(), -1, &font, stroke.points[0], &brush);
        return;
    }

    // Shape drawing
    if (stroke.isShape) {
        if (stroke.points.size() < 2) return;

        Gdiplus::Pen pen(drawColor, stroke.width);
        pen.SetStartCap(Gdiplus::LineCapRound);
        pen.SetEndCap(Gdiplus::LineCapRound);
        pen.SetLineJoin(Gdiplus::LineJoinRound);

        Gdiplus::PointF start = stroke.points[0];
        Gdiplus::PointF end = stroke.points[1];

        if (stroke.shapeType == ToolType::LINE) {
            graphics.DrawLine(&pen, start, end);
        } else if (stroke.shapeType == ToolType::ARROW) {
            graphics.DrawLine(&pen, start, end);
            DrawArrowhead(graphics, pen, start, end);
        } else if (stroke.shapeType == ToolType::RECTANGLE) {
            float x = (std::min)(start.X, end.X);
            float y = (std::min)(start.Y, end.Y);
            float w = std::abs(start.X - end.X);
            float h = std::abs(start.Y - end.Y);
            graphics.DrawRectangle(&pen, x, y, w, h);
        } else if (stroke.shapeType == ToolType::ELLIPSE) {
            float x = (std::min)(start.X, end.X);
            float y = (std::min)(start.Y, end.Y);
            float w = std::abs(start.X - end.X);
            float h = std::abs(start.Y - end.Y);
            graphics.DrawEllipse(&pen, x, y, w, h);
        }
        return;
    }

    // Freehand drawing (PEN / HIGHLIGHTER)
    Gdiplus::Pen pen(drawColor, stroke.width);
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);

    if (stroke.points.size() == 1) {
        Gdiplus::SolidBrush brush(drawColor);
        float r = stroke.width / 2.0f;
        graphics.FillEllipse(&brush, stroke.points[0].X - r, stroke.points[0].Y - r, stroke.width, stroke.width);
    } else if (stroke.points.size() == 2) {
        graphics.DrawLine(&pen, stroke.points[0], stroke.points[1]);
    } else {
        graphics.DrawCurve(&pen, stroke.points.data(), static_cast<INT>(stroke.points.size()));
    }
}

void DrawingEngine::DrawArrowhead(Gdiplus::Graphics& graphics, const Gdiplus::Pen& pen, const Gdiplus::PointF& start, const Gdiplus::PointF& end) {
    float dx = end.X - start.X;
    float dy = end.Y - start.Y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return;

    float ux = dx / len;
    float uy = dy / len;

    // Arrowhead size adjustments based on pen width
    float arrowLength = 14.0f + pen.GetWidth() * 0.5f;
    float arrowWidth = 7.0f + pen.GetWidth() * 0.5f;

    Gdiplus::PointF base(end.X - ux * arrowLength, end.Y - uy * arrowLength);

    // Normal vector
    float nx = -uy;
    float ny = ux;

    Gdiplus::PointF p1(base.X + nx * arrowWidth, base.Y + ny * arrowWidth);
    Gdiplus::PointF p2(base.X - nx * arrowWidth, base.Y - ny * arrowWidth);

    Gdiplus::PointF points[3] = { end, p1, p2 };

    Gdiplus::Color color;
    pen.GetColor(&color);
    Gdiplus::SolidBrush brush(color);
    graphics.FillPolygon(&brush, points, 3);
}
