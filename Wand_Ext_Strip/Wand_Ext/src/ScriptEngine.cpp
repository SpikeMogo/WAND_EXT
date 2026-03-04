// ScriptEngine.cpp
#include "ScriptEngine.h"
#include <iostream>
#include <unordered_set>

#ifdef _DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) ((void)0)
#endif

static const std::unordered_set<std::string> kBuiltinModules = {
    "_G", "package", "string", "table", "math",
    "io", "os", "debug", "coroutine", "utf8"
};

// ============================================================================
// Custom allocator with memory limit
// ============================================================================

void* ScriptEngine::LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    ScriptEngine* self = static_cast<ScriptEngine*>(ud);

    // Per Lua docs: when ptr == NULL, osize encodes the object type, not a size.
    // Only treat osize as a real previous-allocation size when ptr != NULL.
    size_t realOldSize = (ptr != nullptr) ? osize : 0;

    if (nsize == 0) {
        // Free
        if (self->currentAlloc_ >= realOldSize)
            self->currentAlloc_ -= realOldSize;
        else
            self->currentAlloc_ = 0;  // clamp to avoid underflow
        free(ptr);
        return nullptr;
    }

    // Check limit (only for growth)
    // Allow small allocations (< 256 bytes) even over the limit so Lua can
    // build the "not enough memory" error message and unwind via luaL_error
    // instead of panicking.  Without this headroom the allocator denies the
    // error-string allocation itself, causing an unrecoverable lua_atpanic.
    static constexpr size_t kErrorHeadroom = 256;

    if (nsize > realOldSize) {
        size_t growth = nsize - realOldSize;
        if (self->currentAlloc_ + growth > self->memoryLimit_) {
            if (nsize > kErrorHeadroom)
                return nullptr;  // allocation denied — Lua will raise out-of-memory
            // else: allow it so error handling works
        }
    }

    void* newPtr = realloc(ptr, nsize);
    if (newPtr) {
        self->currentAlloc_ += nsize;
        if (self->currentAlloc_ >= realOldSize)
            self->currentAlloc_ -= realOldSize;
        else
            self->currentAlloc_ = nsize;  // fresh allocation, shouldn't happen but safe
    }
    return newPtr;
}

// ============================================================================
// Constructor
// ============================================================================

ScriptEngine::ScriptEngine()
    : lua(sol::default_at_panic, LuaAlloc, this)
{
    lua.open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::math,
        sol::lib::table,
        sol::lib::io,
        sol::lib::os,
        sol::lib::debug,
        sol::lib::utf8
    );
    BindLogging();
}

void ScriptEngine::BindLogging()
{
    lua.set_function("log", [this](const std::string& msg) {
        if (logCallback)
            logCallback(msg);
        else
            DEBUG_PRINTF("[Lua] %s\n", msg.c_str());
    });
}

// ============================================================================
// Cancel hook — uses extraspace for fast access (no registry lookup)
// ============================================================================

void ScriptEngine::SetCancelFlag(std::atomic<bool>* flag)
{
    // Store flag pointer in Lua extraspace (set up by SetupLuaEnvironment)
    // Also store in registry as a fallback
    lua_pushlightuserdata(lua.lua_state(), flag);
    lua_setfield(lua.lua_state(), LUA_REGISTRYINDEX, "__cancel_flag");

    lua_sethook(lua.lua_state(), [](lua_State* L, lua_Debug*) {
        // Fast path: read from registry
        lua_getfield(L, LUA_REGISTRYINDEX, "__cancel_flag");
        auto* f = static_cast<std::atomic<bool>*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (f && f->load(std::memory_order_relaxed)) {
            luaL_error(L, "script cancelled");
        }
    }, LUA_MASKCOUNT, 5000);
}

void ScriptEngine::ClearCancelHook()
{
    lua_sethook(lua.lua_state(), nullptr, 0, 0);
}

// ============================================================================
// Memory diagnostics
// ============================================================================

size_t ScriptEngine::GetMemoryUsage() const
{
    lua_State* L = lua.lua_state();
    return static_cast<size_t>(lua_gc(L, LUA_GCCOUNT, 0)) * 1024
         + static_cast<size_t>(lua_gc(L, LUA_GCCOUNTB, 0));
}

void ScriptEngine::ForceGC()
{
    lua.collect_garbage();
}

// ============================================================================
// Global cleanup between runs
// ============================================================================

void ScriptEngine::SnapshotGlobals()
{
    knownGlobals_.clear();
    sol::table globals = lua.globals();
    for (auto& pair : globals) {
        if (pair.first.is<std::string>()) {
            knownGlobals_.insert(pair.first.as<std::string>());
        }
    }
}

void ScriptEngine::CleanUserGlobals()
{
    if (knownGlobals_.empty())
        return;  // no snapshot taken yet

    sol::table globals = lua.globals();
    std::vector<std::string> toRemove;

    for (auto& pair : globals) {
        if (pair.first.is<std::string>()) {
            std::string name = pair.first.as<std::string>();
            if (knownGlobals_.find(name) == knownGlobals_.end()) {
                toRemove.push_back(name);
            }
        }
    }

    for (const auto& name : toRemove)
        globals[name] = sol::nil;

    if (!toRemove.empty() && logCallback) {
        logCallback("[Mem] Cleaned " + std::to_string(toRemove.size()) + " user globals");
    }
}

// ============================================================================
// Initialize
// ============================================================================

void ScriptEngine::Initialize()
{
    lua_gc(lua.lua_state(), LUA_GCSETPAUSE, 110);
    lua_gc(lua.lua_state(), LUA_GCSETSTEPMUL, 400);

    lua.set_function("print", [this](sol::variadic_args args) {
        std::string result;
        for (auto v : args) {
            sol::object obj = v;
            if (obj.is<std::string>())
                result += obj.as<std::string>();
            else if (obj.is<double>())
                result += std::to_string(obj.as<double>());
            else if (obj.is<bool>())
                result += obj.as<bool>() ? "true" : "false";
            else if (obj.is<sol::nil_t>())
                result += "nil";
            else
                result += "[object]";
            result += " ";
        }

        if (logCallback)
            logCallback(result);
        else
            DEBUG_PRINTF("[Lua] %s\n", result.c_str());
    });

    BindLogging();
}

// ============================================================================
// RunFile
// ============================================================================

bool ScriptEngine::RunFile(const std::string& path)
{
    // Clean up leftovers from previous run
    CleanUserGlobals();

    ForceGC();
    size_t memBefore = GetMemoryUsage();

    auto logMemory = [&](const char* status) {
        ForceGC();
        size_t memAfter = GetMemoryUsage();
        int64_t delta = static_cast<int64_t>(memAfter) - static_cast<int64_t>(memBefore);

        if (logCallback) {
            char buf[128];
            snprintf(buf, sizeof(buf), "[Mem] %s - Before: %zu KB, After: %zu KB (%+lld KB)",
                     status, memBefore / 1024, memAfter / 1024, delta / 1024);
            logCallback(buf);
        }
    };

    // Clear module cache
    sol::table loaded = lua["package"]["loaded"];
    std::vector<std::string> toClear;

    for (auto& pair : loaded) {
        if (pair.first.is<std::string>()) {
            std::string name = pair.first.as<std::string>();
            if (kBuiltinModules.find(name) == kBuiltinModules.end())
                toClear.push_back(name);
        }
    }

    for (const auto& name : toClear)
        loaded[name] = sol::nil;

    if (logCallback)
        logCallback("[Mem] Cleared " + std::to_string(toClear.size()) + " cached modules");

    // Load script
    sol::load_result chunk = lua.load_file(path);
    if (!chunk.valid()) {
        sol::error err = chunk;
        std::string msg = std::string("load_file error: ") + err.what();

        if (logCallbackE)
            logCallbackE(msg);
        else
            std::cerr << msg << "\n";

        logMemory("LoadError");
        return false;
    }

    // Execute script
    sol::protected_function_result result = chunk();
    if (!result.valid()) {
        sol::error err = result;
        std::string errMsg = err.what();

        // Don't treat cancellation as an error
        if (errMsg.find("script cancelled") != std::string::npos) {
            logMemory("Cancelled");
            return false;
        }

        // Handle out-of-memory: log clearly, run GC to free space
        if (errMsg.find("not enough memory") != std::string::npos) {
            ForceGC();
            if (logCallbackE)
                logCallbackE("[Lua] Out of memory (limit: " +
                             std::to_string(memoryLimit_ / (1024*1024)) + " MB)");
            logMemory("OutOfMemory");
            return false;
        }

        std::string msg = std::string("runtime error: ") + errMsg;

        if (logCallbackE)
            logCallbackE(msg);
        else
            std::cerr << msg << "\n";

        logMemory("RuntimeError");
        return false;
    }

    logMemory("Success");
    return true;
}

// ============================================================================
// RunString
// ============================================================================

bool ScriptEngine::RunString(const std::string& code)
{
    sol::load_result chunk = lua.load(code);
    if (!chunk.valid()) {
        sol::error err = chunk;
        std::string msg = std::string("load error: ") + err.what();

        if (logCallbackE)
            logCallbackE(msg);
        else
            std::cerr << msg << "\n";
        return false;
    }

    sol::protected_function_result result = chunk();
    if (!result.valid()) {
        sol::error err = result;
        std::string msg = std::string("runtime error: ") + err.what();

        if (logCallbackE)
            logCallbackE(msg);
        else
            std::cerr << msg << "\n";

        ForceGC();
        return false;
    }

    ForceGC();
    return true;
}
