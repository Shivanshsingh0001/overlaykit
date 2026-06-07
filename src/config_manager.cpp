#include "config_manager.h"
#include <shlobj.h>
#include <fstream>
#include <algorithm>
#include <sstream>

std::wstring ConfigManager::GetConfigFilePath() {
    wchar_t appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
        std::wstring configDir = std::wstring(appData) + L"\\OverlayKit";
        
        // Ensure directory exists
        CreateDirectoryW(configDir.c_str(), nullptr);
        
        return configDir + L"\\config.json";
    }
    return L"config.json"; // Fallback to current directory
}

bool ConfigManager::Save(const AppConfig& config) {
    std::wstring filePath = GetConfigFilePath();
    std::wofstream out(filePath.c_str());
    if (!out.is_open()) return false;

    out << L"{\n";
    out << L"  \"hotkey_toggle\": \"" << config.hotkey_toggle << L"\",\n";
    out << L"  \"hotkey_clear\": \"" << config.hotkey_clear << L"\",\n";
    out << L"  \"hotkey_undo\": \"" << config.hotkey_undo << L"\",\n";
    out << L"  \"default_tool\": \"" << config.default_tool << L"\",\n";
    out << L"  \"default_color\": \"" << config.default_color << L"\",\n";
    out << L"  \"default_brush_size\": " << config.default_brush_size << L",\n";
    out << L"  \"toolbar_x\": " << config.toolbar_x << L",\n";
    out << L"  \"toolbar_y\": " << config.toolbar_y << L",\n";
    out << L"  \"start_with_windows\": " << (config.start_with_windows ? L"true" : L"false") << L",\n";
    out << L"  \"overlay_opacity\": " << config.overlay_opacity << L",\n";
    out << L"  \"license_name\": \"" << config.license_name << L"\",\n";
    out << L"  \"license_key\": \"" << config.license_key << L"\"\n";
    out << L"}\n";

    out.close();
    return true;
}

bool ConfigManager::Load(AppConfig& config) {
    std::wstring filePath = GetConfigFilePath();
    std::wifstream in(filePath.c_str());
    if (!in.is_open()) {
        // Re-write defaults if file not present
        Save(config);
        return true;
    }

    std::wstring line;
    while (std::getline(in, line)) {
        size_t colon = line.find(L':');
        if (colon == std::wstring::npos) continue;

        std::wstring key = line.substr(0, colon);
        std::wstring val = line.substr(colon + 1);

        // Trim whitespace and quotes from key
        key.erase(std::remove(key.begin(), key.end(), L' '), key.end());
        key.erase(std::remove(key.begin(), key.end(), L'\"'), key.end());
        key.erase(std::remove(key.begin(), key.end(), L'\t'), key.end());

        // Extract value inside quotes, or trim raw characters for numeric/bool
        size_t firstQuote = val.find(L'\"');
        size_t lastQuote = val.rfind(L'\"');
        
        std::wstring cleanVal;
        if (firstQuote != std::wstring::npos && lastQuote != std::wstring::npos && firstQuote != lastQuote) {
            cleanVal = val.substr(firstQuote + 1, lastQuote - firstQuote - 1);
        } else {
            cleanVal = val;
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), L' '), cleanVal.end());
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), L'\t'), cleanVal.end());
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), L','), cleanVal.end());
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), L'\r'), cleanVal.end());
            cleanVal.erase(std::remove(cleanVal.begin(), cleanVal.end(), L'\n'), cleanVal.end());
        }

        // Apply fields
        if (key == L"hotkey_toggle") config.hotkey_toggle = cleanVal;
        else if (key == L"hotkey_clear") config.hotkey_clear = cleanVal;
        else if (key == L"hotkey_undo") config.hotkey_undo = cleanVal;
        else if (key == L"default_tool") config.default_tool = cleanVal;
        else if (key == L"default_color") config.default_color = cleanVal;
        else if (key == L"default_brush_size") {
            try {
                config.default_brush_size = std::stof(cleanVal);
            } catch (...) {}
        }
        else if (key == L"toolbar_x") {
            try {
                config.toolbar_x = std::stoi(cleanVal);
            } catch (...) {}
        }
        else if (key == L"toolbar_y") {
            try {
                config.toolbar_y = std::stoi(cleanVal);
            } catch (...) {}
        }
        else if (key == L"start_with_windows") {
            config.start_with_windows = (cleanVal == L"true");
        }
        else if (key == L"overlay_opacity") {
            try {
                config.overlay_opacity = std::stoi(cleanVal);
            } catch (...) {}
        }
        else if (key == L"license_name") config.license_name = cleanVal;
        else if (key == L"license_key") config.license_key = cleanVal;
    }

    in.close();

    // Verify key integrity
    config.is_licensed = VerifyLicenseKey(config.license_name, config.license_key);

    return true;
}

unsigned int ConfigManager::ComputeLicenseHash(const std::wstring& name) {
    unsigned int hash = 5381;
    for (wchar_t c : name) {
        hash = ((hash << 5) + hash) + c; // djb2 standard
    }
    return hash ^ 0x5ECA1EAB; // Xor with salt
}

bool ConfigManager::VerifyLicenseKey(const std::wstring& name, const std::wstring& key) {
    if (name.empty() || key.empty()) return false;

    std::wstring cleanKey = key;
    cleanKey.erase(std::remove(cleanKey.begin(), cleanKey.end(), L' '), cleanKey.end());
    cleanKey.erase(std::remove(cleanKey.begin(), cleanKey.end(), L'\t'), cleanKey.end());

    if (cleanKey.find(L"OK-") != 0) return false;
    std::wstring hashPart = cleanKey.substr(3);

    unsigned int expectedHash = ComputeLicenseHash(name);
    wchar_t hexStr[32];
    swprintf_s(hexStr, L"%08X", expectedHash);

    return _wcsicmp(hashPart.c_str(), hexStr) == 0;
}

std::wstring ConfigManager::GenerateLicenseKey(const std::wstring& name) {
    unsigned int hash = ComputeLicenseHash(name);
    wchar_t keyStr[64];
    swprintf_s(keyStr, L"OK-%08X", hash);
    return std::wstring(keyStr);
}
