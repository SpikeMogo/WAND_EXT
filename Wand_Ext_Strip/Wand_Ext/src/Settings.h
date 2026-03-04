// Settings.h
#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <stdexcept>
#include <functional>

class Settings {
public:
    using Value = std::variant<int, double, bool, std::string>;

    static Settings& Instance() {
        static Settings instance;
        return instance;
    }

    // Register with default (call once at startup)
    void Register(const std::string& key, Value defaultVal);

    // Getters
    int GetInt(const std::string& key);
    double GetDouble(const std::string& key);
    bool GetBool(const std::string& key);
    std::string GetString(const std::string& key);

    // Setters
    void Set(const std::string& key, int value);
    void Set(const std::string& key, double value);
    void Set(const std::string& key, bool value);
    void Set(const std::string& key, const std::string& value);

    // Reset
    void ResetToDefault(const std::string& key);
    void ResetAll();
    bool IsDefault(const std::string& key);

    // File I/O
    void Load(const std::string& filepath = "settings.ini");
    void Save();

    // ImGui helpers with reset button
    bool SliderInt(const char* label, const std::string& key, int min, int max, const char* tooltip = nullptr);
    bool SliderDouble(const char* label, const std::string& key, double min, double max, const char* tooltip = nullptr);
    bool InputInt(const char* label, const std::string& key, const char* tooltip = nullptr);
    bool InputDouble(const char* label, const std::string& key, const char* tooltip = nullptr);
    bool InputText(const char* label, const std::string& key, bool password = false, const char* tooltip = nullptr);
    bool InputTextWithRandom(const char* label, const std::string& key, std::function<std::string()> randomGenerator, const char* tooltip = nullptr);
    bool Checkbox(const char* label, const std::string& key, const char* tooltip = nullptr);
    bool HotkeyInput(const char* label, const std::string& key, const char* tooltip = nullptr);

    // True while the hotkey picker is in capture mode (suppresses global hotkey polling)
    static bool IsCapturingHotkey();

    const std::string& GetFilePath() const { return filepath_; }
    std::string GetFullPath() const;

private:
    void DrawTooltip(const char* tooltip);

private:
    Settings() = default;
    static std::string GetExeDirectory();
    
    struct Entry {
        Value current;
        Value defaultVal;
    };
    
    std::unordered_map<std::string, Entry> data_;
    std::string filepath_ = "settings.ini";

    Entry& GetEntry(const std::string& key);
    bool DrawResetButton(const std::string& key);
};