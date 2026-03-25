#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NinjaEngine {

// Engine state
enum class EngineState {
    UNINITIALIZED,
    INITIALIZING,
    READY,
    EXECUTING,
    ERROR
};

// Script execution result
struct ExecutionResult {
    bool success;
    std::string output;
    std::string error;
    double executionTime;
};

// Lua execution context
class LuaContext {
public:
    virtual ~LuaContext() = default;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual ExecutionResult ExecuteScript(const std::string& script) = 0;
    virtual bool IsReady() const = 0;
};

// Memory utilities for Roblox process interaction
class MemoryManager {
public:
    static bool AttachToProcess(DWORD processId);
    static void Detach();
    static bool IsAttached();
    static bool ReadMemory(LPVOID address, LPVOID buffer, SIZE_T size);
    static bool WriteMemory(LPVOID address, LPCVOID buffer, SIZE_T size);
    static LPVOID AllocateMemory(SIZE_T size);
    static bool FreeMemory(LPVOID address);
    static DWORD GetRobloxProcessId();
    
private:
    static HANDLE s_processHandle;
    static DWORD s_processId;
};

// Communication pipe for UI <-> Engine communication
class CommunicationPipe {
public:
    bool Create(const std::string& pipeName);
    void Close();
    bool IsConnected() const;
    bool SendMessage(const std::string& message);
    std::string ReceiveMessage();
    
private:
    HANDLE m_pipeHandle = INVALID_HANDLE_VALUE;
    bool m_connected = false;
};

// Main engine class
class ExecutionEngine {
public:
    ExecutionEngine();
    ~ExecutionEngine();
    
    bool Initialize();
    void Shutdown();
    
    EngineState GetState() const { return m_state; }
    
    // Script execution
    ExecutionResult ExecuteScript(const std::string& script);
    void StopExecution();
    
    // Communication
    void SetMessageCallback(std::function<void(const std::string&)> callback);
    void SendOutput(const std::string& output);
    
    // Process injection
    bool InjectIntoRoblox();
    bool IsInjected() const { return m_injected; }
    
private:
    EngineState m_state;
    bool m_injected;
    std::unique_ptr<LuaContext> m_luaContext;
    std::function<void(const std::string&)> m_messageCallback;
    CommunicationPipe m_pipe;
    
    void SetState(EngineState state);
    void InitializeLuaEnvironment();
};

// DLL exports
extern "C" {
    __declspec(dllexport) bool InitializeEngine();
    __declspec(dllexport) void ShutdownEngine();
    __declspec(dllexport) bool ExecuteScript(const char* script, char* output, size_t outputSize);
    __declspec(dllexport) bool IsEngineReady();
    __declspec(dllexport) const char* GetEngineVersion();
}

// Global engine instance
extern ExecutionEngine* g_engine;

} // namespace NinjaEngine
