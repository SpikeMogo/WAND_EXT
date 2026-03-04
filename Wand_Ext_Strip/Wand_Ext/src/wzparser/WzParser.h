// WzParser.h — Stripped for open-source release
// Implement your own WZ file parser here.
// The original supported automatic IV/version detection and
// Data folder mode for MapleStory WZ files.
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

// Debug macro stub
#ifndef WZ_DEBUG
#define WZ_DEBUG(tag, fmt, ...)
#endif

namespace wz {

// Minimal type stubs — implement your own WZ node/file types
class WzNode {
public:
    const std::string& name() const { static std::string empty; return empty; }
    WzNode* get_child(const std::string& name) { return nullptr; }
    const std::vector<std::unique_ptr<WzNode>>& children() const { return m_children; }
    size_t child_count() const { return 0; }
    bool is_directory() const { return false; }
    void clear_children() {}

    std::string get_string() const { return {}; }
    int32_t get_int(int32_t def = 0) const { return def; }

private:
    std::vector<std::unique_ptr<WzNode>> m_children;
};

class WzFile {
public:
    bool open(const std::string& path) { return false; }
    void close() {}
    void set_game_pid(DWORD pid) {}
    WzNode* root() { return &m_root; }
    void parse_image(WzNode* node) {}

private:
    WzNode m_root;
};

} // namespace wz
