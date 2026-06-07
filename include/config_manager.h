#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>

struct AppConfig {
    std::wstring hotkey_toggle = L"Ctrl+Alt+D";
    std::wstring hotkey_clear = L"Ctrl+Shift+C";
    std::wstring hotkey_undo = L"Ctrl+Z";
    std::wstring default_tool = L"pen";
    std::wstring default_color = L"#FF0000";
    float default_brush_size = 4.0f;
    int toolbar_x = 100;
    int toolbar_y = 200;
    bool start_with_windows = false;
    int overlay_opacity = 100;
    std::wstring license_name = L"";
    std::wstring license_key = L"";
    bool is_licensed = false;
};

class ConfigManager {
public:
    static bool Load(AppConfig& config);
    static bool Save(const AppConfig& config);
    static std::wstring GetConfigFilePath();

    // Offline License Management helper functions
    static unsigned int ComputeLicenseHash(const std::wstring& name);
    static bool VerifyLicenseKey(const std::wstring& name, const std::wstring& key);
    static std::wstring GenerateLicenseKey(const std::wstring& name);
};

#endif // CONFIG_MANAGER_H
