// Hotkey.h — Global hotkey encoding, decoding, and display helpers
#pragma once

#include <Windows.h>
#include <string>

// Encoding layout: [31..16] modifier flags | [15..0] VK code
// Value 0 = no hotkey assigned
constexpr int HK_MOD_CTRL  = 0x00010000;
constexpr int HK_MOD_SHIFT = 0x00020000;
constexpr int HK_MOD_ALT   = 0x00040000;
constexpr int HK_VK_MASK   = 0x0000FFFF;

inline int HotkeyEncode(int vk, bool ctrl, bool shift, bool alt) {
    int val = vk & HK_VK_MASK;
    if (ctrl)  val |= HK_MOD_CTRL;
    if (shift) val |= HK_MOD_SHIFT;
    if (alt)   val |= HK_MOD_ALT;
    return val;
}

inline int  HotkeyVK(int packed)    { return packed & HK_VK_MASK; }
inline bool HotkeyCtrl(int packed)  { return (packed & HK_MOD_CTRL)  != 0; }
inline bool HotkeyShift(int packed) { return (packed & HK_MOD_SHIFT) != 0; }
inline bool HotkeyAlt(int packed)   { return (packed & HK_MOD_ALT)   != 0; }

inline const char* VKToName(int vk) {
    switch (vk) {
        case 0:             return "None";
        // Function keys
        case VK_F1:         return "F1";
        case VK_F2:         return "F2";
        case VK_F3:         return "F3";
        case VK_F4:         return "F4";
        case VK_F5:         return "F5";
        case VK_F6:         return "F6";
        case VK_F7:         return "F7";
        case VK_F8:         return "F8";
        case VK_F9:         return "F9";
        case VK_F10:        return "F10";
        case VK_F11:        return "F11";
        case VK_F12:        return "F12";
        case VK_F13:        return "F13";
        case VK_F14:        return "F14";
        case VK_F15:        return "F15";
        case VK_F16:        return "F16";
        case VK_F17:        return "F17";
        case VK_F18:        return "F18";
        case VK_F19:        return "F19";
        case VK_F20:        return "F20";
        case VK_F21:        return "F21";
        case VK_F22:        return "F22";
        case VK_F23:        return "F23";
        case VK_F24:        return "F24";
        // Navigation
        case VK_INSERT:     return "Insert";
        case VK_DELETE:     return "Delete";
        case VK_HOME:       return "Home";
        case VK_END:        return "End";
        case VK_PRIOR:      return "PageUp";
        case VK_NEXT:       return "PageDown";
        // Special
        case VK_ESCAPE:     return "Esc";
        case VK_RETURN:     return "Enter";
        case VK_SPACE:      return "Space";
        case VK_BACK:       return "Backspace";
        case VK_TAB:        return "Tab";
        case VK_CAPITAL:    return "CapsLock";
        case VK_PAUSE:      return "Pause";
        case VK_SCROLL:     return "ScrollLock";
        case VK_SNAPSHOT:   return "PrintScreen";
        case VK_NUMLOCK:    return "NumLock";
        // Numpad
        case VK_NUMPAD0:    return "Num0";
        case VK_NUMPAD1:    return "Num1";
        case VK_NUMPAD2:    return "Num2";
        case VK_NUMPAD3:    return "Num3";
        case VK_NUMPAD4:    return "Num4";
        case VK_NUMPAD5:    return "Num5";
        case VK_NUMPAD6:    return "Num6";
        case VK_NUMPAD7:    return "Num7";
        case VK_NUMPAD8:    return "Num8";
        case VK_NUMPAD9:    return "Num9";
        case VK_MULTIPLY:   return "Num*";
        case VK_ADD:        return "Num+";
        case VK_SUBTRACT:   return "Num-";
        case VK_DECIMAL:    return "Num.";
        case VK_DIVIDE:     return "Num/";
        // Arrow keys
        case VK_LEFT:       return "Left";
        case VK_UP:         return "Up";
        case VK_RIGHT:      return "Right";
        case VK_DOWN:       return "Down";
        // Letters
        default:
            if (vk >= 'A' && vk <= 'Z') {
                static char letter[2] = {};
                letter[0] = (char)vk;
                return letter;
            }
            if (vk >= '0' && vk <= '9') {
                static char digit[2] = {};
                digit[0] = (char)vk;
                return digit;
            }
            // Fallback: use Windows key name
            {
                UINT scancode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
                if (scancode) {
                    static char name[32] = {};
                    if (GetKeyNameTextA((LONG)(scancode << 16), name, sizeof(name)) > 0)
                        return name;
                }
            }
            return "?";
    }
}

inline std::string HotkeyToString(int packed) {
    if (packed == 0) return "None";
    std::string s;
    if (HotkeyCtrl(packed))  s += "Ctrl+";
    if (HotkeyShift(packed)) s += "Shift+";
    if (HotkeyAlt(packed))   s += "Alt+";
    s += VKToName(HotkeyVK(packed));
    return s;
}

// Check if a hotkey matches the current keyboard state (with edge detection)
inline bool PollHotkey(int packed, bool& prevDown) {
    if (packed == 0) { prevDown = false; return false; }

    int vk = HotkeyVK(packed);
    bool needCtrl  = HotkeyCtrl(packed);
    bool needShift = HotkeyShift(packed);
    bool needAlt   = HotkeyAlt(packed);

    bool vkDown    = (GetAsyncKeyState(vk)         & 0x8000) != 0;
    bool ctrlDown  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
    bool altDown   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;

    bool isDown = vkDown &&
                  (ctrlDown  == needCtrl) &&
                  (shiftDown == needShift) &&
                  (altDown   == needAlt);

    bool edge = isDown && !prevDown;
    prevDown = isDown;
    return edge;
}
