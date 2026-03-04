// Settings.cpp
#include "Settings.h"
#include "Hotkey.h"
#include "imgui.h"
#include <filesystem>
#include <Windows.h>

std::string Settings::GetExeDirectory() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    return std::filesystem::path(exePath).parent_path().string();
}

std::string Settings::GetFullPath() const {
    return std::filesystem::absolute(filepath_).string();
}

void Settings::Register(const std::string& key, Value defaultVal) {
    if (data_.find(key) == data_.end()) {
        data_[key] = { defaultVal, defaultVal };
    } else {
        data_[key].defaultVal = defaultVal;
    }
}

Settings::Entry& Settings::GetEntry(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        throw std::runtime_error("Setting not registered: " + key);
    }
    return it->second;
}

int Settings::GetInt(const std::string& key) {
    return std::get<int>(GetEntry(key).current);
}

double Settings::GetDouble(const std::string& key) {
    auto& val = GetEntry(key).current;
    if (std::holds_alternative<double>(val)) return std::get<double>(val);
    if (std::holds_alternative<int>(val)) return static_cast<double>(std::get<int>(val));
    throw std::runtime_error("Type mismatch for: " + key);
}

bool Settings::GetBool(const std::string& key) {
    return std::get<bool>(GetEntry(key).current);
}

std::string Settings::GetString(const std::string& key) {
    return std::get<std::string>(GetEntry(key).current);
}

void Settings::Set(const std::string& key, int value) {
    GetEntry(key).current = value;
    Save();
}

void Settings::Set(const std::string& key, double value) {
    GetEntry(key).current = value;
    Save();
}

void Settings::Set(const std::string& key, bool value) {
    GetEntry(key).current = value;
    Save();
}

void Settings::Set(const std::string& key, const std::string& value) {
    GetEntry(key).current = value;
    Save();
}

void Settings::ResetToDefault(const std::string& key) {
    auto& entry = GetEntry(key);
    entry.current = entry.defaultVal;
    Save();
}

void Settings::ResetAll() {
    for (auto& [key, entry] : data_) {
        entry.current = entry.defaultVal;
    }
    Save();
}

bool Settings::IsDefault(const std::string& key) {
    auto& entry = GetEntry(key);
    if (entry.current.index() != entry.defaultVal.index())
        return false;
    return entry.current == entry.defaultVal;
}

void Settings::Load(const std::string& filename) {
    filepath_ = GetExeDirectory() + "\\" + filename;
    
    std::ifstream file(filepath_);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);

        if (val.size() > 2 && val[1] == ':') {
            char type = val[0];
            std::string data = val.substr(2);
            Value parsed;
            switch (type) {
                case 'i': parsed = std::stoi(data); break;
                case 'd': parsed = std::stod(data); break;
                case 'b': parsed = (data == "1" || data == "true"); break;
                case 's': parsed = data; break;
                default: continue;
            }
            data_[key] = { parsed, parsed };
        }
    }
}

void Settings::Save() {
    std::ofstream file(filepath_);
    if (!file.is_open()) return;

    file << "# Bot Settings\n\n";
    for (const auto& [key, entry] : data_) {
        file << key << "=";
        std::visit([&file](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>) file << "i:" << arg;
            else if constexpr (std::is_same_v<T, double>) file << "d:" << arg;
            else if constexpr (std::is_same_v<T, bool>) file << "b:" << (arg ? "1" : "0");
            else if constexpr (std::is_same_v<T, std::string>) file << "s:" << arg;
        }, entry.current);
        file << "\n";
    }
}

bool Settings::DrawResetButton(const std::string& key) {
    bool isDefault = IsDefault(key);
    
    ImGui::SameLine();
    ImGui::PushID(key.c_str());
    
    if (isDefault)
        ImGui::BeginDisabled();
    
    bool clicked = ImGui::SmallButton("Reset");
    
    if (isDefault) {
        ImGui::EndDisabled();
    } else if (ImGui::IsItemHovered()) {
        auto& entry = GetEntry(key);
        std::visit([](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>)
                ImGui::SetTooltip("Default: %d", arg);
            else if constexpr (std::is_same_v<T, double>)
                ImGui::SetTooltip("Default: %.2f", arg);
            else if constexpr (std::is_same_v<T, bool>)
                ImGui::SetTooltip("Default: %s", arg ? "true" : "false");
            else if constexpr (std::is_same_v<T, std::string>)
                ImGui::SetTooltip("Default: %s", arg.c_str());
        }, entry.defaultVal);
    }
    
    ImGui::PopID();
    
    if (clicked) {
        ResetToDefault(key);
        return true;
    }
    return false;
}

void Settings::DrawTooltip(const char* tooltip) {
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(360.0f);
        ImGui::TextUnformatted(tooltip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool Settings::SliderInt(const char* label, const std::string& key, int min, int max, const char* tooltip) {
    int val = GetInt(key);
    
    bool changed = ImGui::SliderInt(label, &val, min, max);
    DrawTooltip(tooltip);
    
    if (changed)
        GetEntry(key).current = val;
    if (ImGui::IsItemDeactivatedAfterEdit())
        Save();
    
    bool reset = DrawResetButton(key);
    return changed || reset;
}

bool Settings::SliderDouble(const char* label, const std::string& key, double min, double max, const char* tooltip) {
    float val = static_cast<float>(GetDouble(key));
    
    bool changed = ImGui::SliderFloat(label, &val, static_cast<float>(min), static_cast<float>(max));
    DrawTooltip(tooltip);
    
    if (changed)
        GetEntry(key).current = static_cast<double>(val);
    if (ImGui::IsItemDeactivatedAfterEdit())
        Save();
    
    bool reset = DrawResetButton(key);
    return changed || reset;
}

bool Settings::InputInt(const char* label, const std::string& key, const char* tooltip) {
    int val = GetInt(key);
    
    bool changed = ImGui::InputInt(label, &val);
    DrawTooltip(tooltip);
    
    if (changed)
        GetEntry(key).current = val;
    if (ImGui::IsItemDeactivatedAfterEdit())
        Save();
    
    bool reset = DrawResetButton(key);
    return changed || reset;
}

bool Settings::InputDouble(const char* label, const std::string& key, const char* tooltip) {
    float val = static_cast<float>(GetDouble(key));
    
    bool changed = ImGui::InputFloat(label, &val);
    DrawTooltip(tooltip);
    
    if (changed)
        GetEntry(key).current = static_cast<double>(val);
    if (ImGui::IsItemDeactivatedAfterEdit())
        Save();
    
    bool reset = DrawResetButton(key);
    return changed || reset;
}

bool Settings::InputText(const char* label, const std::string& key, bool password, const char* tooltip) {
    // Use static map to store buffers per key
    static std::unordered_map<std::string, std::string> buffers;
    
    auto it = buffers.find(key);
    if (it == buffers.end()) {
        buffers[key] = GetString(key);
        it = buffers.find(key);
    }
    
    // Resize buffer if needed
    it->second.resize(512);
    
    ImGuiInputTextFlags flags = password ? ImGuiInputTextFlags_Password : 0;
    
    bool changed = ImGui::InputText(label, it->second.data(), it->second.capacity(), flags);
    DrawTooltip(tooltip);
    
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        std::string newVal(it->second.c_str());
        if (newVal != GetString(key)) {
            Set(key, newVal);
            return true;
        }
    }
    
    bool reset = DrawResetButton(key);
    if (reset)
        it->second = GetString(key);
    
    return changed || reset;
}

bool Settings::InputTextWithRandom(const char* label, const std::string& key, std::function<std::string()> randomGenerator, const char* tooltip) {
    static std::unordered_map<std::string, std::string> buffers;
    
    auto it = buffers.find(key);
    if (it == buffers.end()) {
        buffers[key] = GetString(key);
        it = buffers.find(key);
    }
    
    it->second.resize(512);
    
    bool changed = ImGui::InputText(label, it->second.data(), it->second.capacity());
    DrawTooltip(tooltip);
    
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        std::string newVal(it->second.c_str());
        if (newVal != GetString(key)) {
            Set(key, newVal);
            return true;
        }
    }
    
    // Random button instead of Reset
    ImGui::SameLine();
    ImGui::PushID(key.c_str());
    
    if (ImGui::SmallButton("Random")) {
        std::string newVal = randomGenerator();
        Set(key, newVal);
        it->second = newVal;
        ImGui::PopID();
        return true;
    }
    
    ImGui::PopID();
    return changed;
}

bool Settings::Checkbox(const char* label, const std::string& key, const char* tooltip) {
    bool val = GetBool(key);

    bool changed = ImGui::Checkbox(label, &val);
    DrawTooltip(tooltip);

    if (changed)
        Set(key, val);

    bool reset = DrawResetButton(key);
    return changed || reset;
}

// Which settings key is currently in capture mode ("" = none)
static std::string s_hotkeyCapturingKey;

bool Settings::IsCapturingHotkey() {
    return !s_hotkeyCapturingKey.empty();
}

bool Settings::HotkeyInput(const char* label, const std::string& key, const char* tooltip) {
    int val = GetInt(key);
    bool isCapturing = (s_hotkeyCapturingKey == key);

    ImGui::PushID(key.c_str());

    // Fixed width so button doesn't jump around
    float btnWidth = ImGui::CalcTextSize("[Press key or combo (Esc=cancel)]").x + ImGui::GetStyle().FramePadding.x * 2 - 5.0f;

    if (isCapturing) {
        // Capture mode — waiting for key press
        ImGui::Button("[Press key or combo (Esc=cancel)]", ImVec2(btnWidth, 0));

        // Check for cancel (Escape)
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            s_hotkeyCapturingKey.clear();
        }
        // Check for clear (Delete or Backspace)
        else if ((GetAsyncKeyState(VK_DELETE) & 0x8000) || (GetAsyncKeyState(VK_BACK) & 0x8000)) {
            Set(key, 0);
            s_hotkeyCapturingKey.clear();
        }
        else {
            // Scan for any non-modifier key
            for (int vk = 1; vk < 256; vk++) {
                // Skip modifier keys
                if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU ||
                    vk == VK_LCONTROL || vk == VK_RCONTROL ||
                    vk == VK_LSHIFT || vk == VK_RSHIFT ||
                    vk == VK_LMENU || vk == VK_RMENU ||
                    vk == VK_LWIN || vk == VK_RWIN)
                    continue;
                // Skip mouse buttons
                if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON ||
                    vk == VK_XBUTTON1 || vk == VK_XBUTTON2)
                    continue;

                if (GetAsyncKeyState(vk) & 0x8000) {
                    bool ctrl  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool shift = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
                    bool alt   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
                    Set(key, HotkeyEncode(vk, ctrl, shift, alt));
                    s_hotkeyCapturingKey.clear();
                    break;
                }
            }
        }
    } else {
        // Normal mode — show current binding
        char btnLabel[128];
        snprintf(btnLabel, sizeof(btnLabel), "[%s]", HotkeyToString(val).c_str());
        if (ImGui::Button(btnLabel, ImVec2(btnWidth, 0))) {
            s_hotkeyCapturingKey = key;
        }
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    DrawTooltip(tooltip);

    ImGui::PopID();

    bool reset = DrawResetButton(key);
    return reset;
}