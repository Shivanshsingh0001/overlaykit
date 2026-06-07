#ifndef DRAWING_ENGINE_H
#define DRAWING_ENGINE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <vector>
#include <string>

enum class ToolType {
    PEN,
    HIGHLIGHTER,
    ERASER,
    LINE,
    ARROW,
    RECTANGLE,
    ELLIPSE,
    TEXT,
    LASER_POINTER,
    SPOTLIGHT
};

struct Stroke {
    std::vector<Gdiplus::PointF> points;
    Gdiplus::Color color;
    float width;
    bool isHighlighter;
    bool isEraser;
    
    // Shape properties
    bool isShape;
    ToolType shapeType; // LINE, ARROW, RECTANGLE, ELLIPSE

    // Text properties
    std::wstring textContent;
    std::wstring fontName;
    float fontSize;

    // Fading ink properties
    bool isFading;
    float opacity; // 1.0 down to 0.0
    ULONGLONG creationTime; // GetTickCount64()
};

class DrawingEngine {
public:
    DrawingEngine();
    ~DrawingEngine();

    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();

    void Resize(int width, int height);

    // Drawing state triggers
    void StartStroke(float x, float y);
    void AddPointToStroke(float x, float y);
    void EndStroke();

    // Editor operations
    void Undo();
    void Clear();

    // Fading logic tick
    void UpdateFadingStrokes();

    // Text tool integration
    void CommitText(const std::wstring& text, float x, float y, const std::wstring& fontName, float fontSize);

    // Painting
    void Draw(Gdiplus::Graphics& graphics);

    // Active tool state controls
    void SetTool(ToolType tool);
    void SetColor(Gdiplus::Color color);
    void SetBrushWidth(float width);
    void SetWhiteboard(bool enable);
    void SetBlackboard(bool enable);
    void SetFadingInk(bool enable);
    
    // Getters
    ToolType GetTool() const { return m_currentTool; }
    Gdiplus::Color GetColor() const { return m_currentColor; }
    float GetBrushWidth() const { return m_currentWidth; }
    bool IsWhiteboard() const { return m_whiteboardActive; }
    bool IsBlackboard() const { return m_blackboardActive; }
    bool IsFadingInk() const { return m_fadingInkEnabled; }
    bool IsDrawing() const { return m_isDrawing; }

    // Screen tracking for temporary cursors
    void UpdateCursorPosition(float x, float y);
    void SetShowCursorHalo(bool enable);
    bool IsShowCursorHalo() const { return m_showCursorHalo; }

private:
    void RedrawBackgroundCanvas();
    void DrawStrokeToGraphics(Gdiplus::Graphics& graphics, const Stroke& stroke);
    void DrawArrowhead(Gdiplus::Graphics& graphics, const Gdiplus::Pen& pen, const Gdiplus::PointF& start, const Gdiplus::PointF& end);

    HWND m_hwnd;
    int m_width;
    int m_height;

    bool m_isDrawing;
    Stroke m_activeStroke;

    std::vector<Stroke> m_strokeHistory; // Active undo list (max 20)
    std::vector<Stroke> m_fadingStrokes; // Fading ink paths currently decaying
    
    // GDI+ Canvas Bitmaps
    Gdiplus::Bitmap* m_backgroundCanvas; // Offscreen pre-rendered canvas
    Gdiplus::Bitmap* m_permanentCanvas;  // Offscreen pre-rendered canvas for flattened strokes
    
    // Active configuration
    ToolType m_currentTool;
    Gdiplus::Color m_currentColor;
    float m_currentWidth;

    // Display modes
    bool m_whiteboardActive;
    bool m_blackboardActive;
    bool m_fadingInkEnabled;

    // Temporary cursor halos and pointers
    bool m_showCursorHalo;
    Gdiplus::PointF m_cursorPos;

    static const size_t MAX_UNDO_STEPS = 20;
};

#endif // DRAWING_ENGINE_H
