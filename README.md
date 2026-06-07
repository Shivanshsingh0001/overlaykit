# 🖌️ OverlayKit

[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-0078D6.svg?style=flat-square&logo=windows)](https://microsoft.com)
[![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![License: Proprietary / Commercial](https://img.shields.io/badge/License-Proprietary-orange.svg?style=flat-square)](#-licensing)
[![RAM Usage: ~15MB](https://img.shields.io/badge/RAM_Idle-~15MB-green.svg?style=flat-square)](#-performance)

**OverlayKit** is a high-performance, ultra-lightweight, always-on-top transparent presentation overlay tool. 

Whether you are hosting online classes, streaming technical tutorials, presenting slides, or conducting code reviews, OverlayKit lets you draw, annotate, spotlight, and display reference cheat sheets directly on top of your screen in real time.

---

## 🚀 1-Minute Quick Start (For Users)

If you just want to run OverlayKit without dealing with compilation, follow these steps:

1.  **Download the Installer**: Go to the **Releases** tab on GitHub and download the latest `OverlayKitSetup.exe` (built via Inno Setup).
2.  **Run the Setup**: Follow the modern wizard prompts. You can check the option to *"Run OverlayKit automatically when Windows starts"* if desired.
3.  **Launch the App**: Once installed, OverlayKit runs quietly in your System Tray (look for the taskbar icon near your system clock).
4.  **Activate drawing mode**: Press **`Ctrl + Alt + D`**. The mouse cursor will transform into a drawing crosshair, and a floating toolbar will slide onto your screen. You are now ready to draw!
5.  **Return to clicking/typing**: Press **`Ctrl + Alt + D`** again to return to normal mouse interactions (passthrough mode). Your drawings stay visible on top, but clicks go directly through to your desktop applications.

---

## 🌟 Key Features

*   **Global Hotkey Toggle**: Seamlessly switch between drawing mode and clicking-through the transparent overlay via `Ctrl+Alt+D`.
*   **Drawing Toolset**:
    *   *Pen & Highlighter*: Freehand smooth lines with adjustable opacity and custom weights.
    *   *Shapes*: Standard Line, Arrow, Rectangle, and Ellipse.
    *   *Eraser*: Direct per-pixel transparent clearing.
    *   *Text Tool*: Click anywhere, type multiline text, and press enter to overlay it.
*   **Presentation Helpers**:
    *   *Laser Pointer*: Temporary glowing indicator tracking the cursor.
    *   *Spotlight*: Dims the rest of the screen to focus on a circular tracking region.
    *   *Highlighted Cursor*: Circular yellow halo to draw attention to mouse movements.
    *   *Fading Ink*: Draws strokes that automatically fade out after 3 seconds.
    *   *Whiteboard & Blackboard*: Fills the screen with solid backgrounds for diagramming.
    *   *Screenshot*: Capture screen contents (with or without drawings) directly to the clipboard.
*   **Pinned Panels (Cheat Sheets)**:
    *   Spawn up to 5 floating, resizable topmost reference windows.
    *   Drag and drop text files (`.txt`, `.md`, `.json`) or images (`.png`, `.jpg`, `.bmp`, `.gif`) into the panels to display cheat sheets.
    *   Features smooth vertical pixel scrolling via mouse wheel.
    *   **Anti-Capture Protection**: Excluded from screen recording tools (OBS, Zoom, Teams) at the OS level—perfect for private notes you don't want your audience to see.

---

## ⚙️ Building from Source (For Developers)

### 📋 Prerequisites
To build OverlayKit from source, make sure you have:
1.  **Compiler**: MSVC (Visual Studio 2019+) or MinGW-w64 (GCC 10+) supporting C++17.
2.  **Build Tools**: [CMake (3.15+)](https://cmake.org/download/).

### 💻 Step-by-Step Build Commands
1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/yourusername/overlaykit.git
    cd overlaykit
    ```
2.  **Configure CMake**:
    Create a folder named `build` and run the generator:
    ```bash
    cmake -B build -S .
    ```
3.  **Compile the Target**:
    Build in Release mode:
    ```bash
    cmake --build build --config Release
    ```
4.  **Run**:
    Launch the compiled executable:
    ```bash
    ./build/overlaykit.exe
    ```

---

## 🎛️ Configurations & Registry

All application configurations are loaded and saved locally. No network requests are ever made.
*   **Configuration Location**: `%APPDATA%\OverlayKit\config.json`
*   **Startup Toggle**: Handled safely via the registry key `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`. You can enable or disable this via the System Tray context menu or Settings dialog.

---

## 🔑 Offline Licensing

OverlayKit supports local offline activation. Enter your Licensee Name and corresponding License Key in the Settings dialog to activate:
*   The key is validated offline through a custom cryptographic verification algorithm based on a salted DJB2 hash.
*   **License Key Format**: `OK-[HASH-IN-HEX]` (e.g. `OK-ECA1EAB2`)

---

## 📈 Performance

Designed to be ultra-lean and lightweight:
*   **RAM footprint**: Under 15MB when idle; peaks at under 30MB during active screen drawing.
*   **CPU usage**: 0% when idle; less than 3% during active strokes, utilizing a optimized double-buffered bitmap engine.

---

## 🤝 Contributing

Contributions make the open-source and indie-dev community amazing. If you want to contribute:
1. Fork the Project.
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`).
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`).
4. Push to the Branch (`git push origin feature/AmazingFeature`).
5. Open a Pull Request.

---

## 📄 License

OverlayKit is released under a proprietary commercial license. See the `LICENSE` file for more details.
