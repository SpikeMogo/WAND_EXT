// ScriptEngine.h
#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <unordered_set>
#include <sol/sol.hpp>

class ScriptEngine
{
public:
    ScriptEngine();
    ~ScriptEngine() = default;

    void Initialize();
    bool RunFile(const std::string& path);
    bool RunString(const std::string& code);
    void BindLogging();

    void SetCancelFlag(std::atomic<bool>* flag);
    void ClearCancelHook();

    size_t GetMemoryUsage() const;
    size_t GetMemoryLimit() const { return memoryLimit_; }
    size_t GetAllocatedBytes() const { return currentAlloc_; }
    void SetMemoryLimit(size_t bytes) { memoryLimit_ = bytes; }
    void ForceGC();

    sol::state& GetState() { return lua; }

    // Call after all bindings are registered to snapshot known globals
    void SnapshotGlobals();

    std::function<void(const std::string&)> logCallback;
    std::function<void(const std::string&)> logCallbackE;

private:
    // IMPORTANT: memoryLimit_ and currentAlloc_ must be declared BEFORE lua
    // so they are initialized before sol::state's constructor calls LuaAlloc.
    size_t memoryLimit_ = 64 * 1024 * 1024;  // 64 MB default limit
    size_t currentAlloc_ = 0;
    sol::state lua;

    // Custom allocator for memory limiting
    static void* LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize);

    // Snapshot known globals after setup to distinguish user globals
    std::unordered_set<std::string> knownGlobals_;

    void CleanUserGlobals();
};