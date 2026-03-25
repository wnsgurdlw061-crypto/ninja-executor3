#pragma once

#include <windows.h>
#include <exception>
#include <functional>
#include <vector>
#include <atomic>

namespace NinjaEngine {

// Exception handling constants
constexpr DWORD EXCEPTION_CONTINUABLE = 0;
constexpr DWORD EXCEPTION_NONCONTINUABLE = 1;

// Custom exception codes
constexpr DWORD EXCEPTION_NINJA_GUARD = 0x4E494E00; // "NIN\0"
constexpr DWORD EXCEPTION_NINJA_HOOK = 0x4E494E01;
constexpr DWORD EXCEPTION_NINJA_PATCH = 0x4E494E02;

// VEH Hook types
enum class VEHHookType {
    GUARD_PAGE,      // PAGE_GUARD exception
    BREAKPOINT,      // INT3 breakpoint
    SINGLE_STEP,     // Single step exception
    ACCESS_VIOLATION // Memory access violation
};

// Hook callback signature
using VEHCallback = std::function<bool(EXCEPTION_POINTERS* ExceptionInfo)>;

// Exception handler info
struct VEHHandler {
    PVOID handler;
    VEHHookType type;
    PVOID targetAddress;
    PVOID originalBytes;
    SIZE_T originalSize;
    VEHCallback callback;
    bool active;
};

/**
 * Vector Exception Handling (VEH) Security Bypass
 * 
 * VEH is a powerful technique for:
 * 1. Intercepting memory access before anti-cheat checks
 * 2. Bypassing memory protection (PAGE_GUARD, PAGE_NOACCESS)
 * 3. Hooking without modifying code (stealth)
 * 4. Handling breakpoints without debugger detection
 */
class VEHSecurityBypass {
public:
    static VEHSecurityBypass& Instance();
    
    // Initialize VEH system
    bool Initialize();
    void Shutdown();
    
    // Add/remove exception handlers
    bool AddGuardPageHook(PVOID address, SIZE_T size, VEHCallback callback);
    bool AddBreakpointHook(PVOID address, VEHCallback callback);
    bool AddSingleStepHook(PVOID address, VEHCallback callback);
    bool RemoveHook(PVOID address);
    
    // Memory protection bypass
    bool BypassMemoryProtection(PVOID address, SIZE_T size);
    bool RestoreMemoryProtection(PVOID address, SIZE_T size);
    
    // Anti-detection
    bool HideFromDebugger();
    bool SpoofDebugRegisters();
    bool ClearHardwareBreakpoints();
    
    // Utility
    bool IsInitialized() const { return m_initialized; }
    size_t GetActiveHookCount() const { return m_handlers.size(); }
    
private:
    VEHSecurityBypass() = default;
    ~VEHSecurityBypass();
    
    VEHSecurityBypass(const VEHSecurityBypass&) = delete;
    VEHSecurityBypass& operator=(const VEHSecurityBypass&) = delete;
    
    // VEH callback (static for Windows API)
    static LONG WINAPI VectoredHandler(EXCEPTION_POINTERS* ExceptionInfo);
    
    // Handler dispatch
    bool HandleGuardPageViolation(EXCEPTION_POINTERS* info);
    bool HandleBreakpoint(EXCEPTION_POINTERS* info);
    bool HandleSingleStep(EXCEPTION_POINTERS* info);
    bool HandleAccessViolation(EXCEPTION_POINTERS* info);
    bool HandleGenericException(EXCEPTION_POINTERS* info);
    
    // Anti-cheat evasion
    bool SpoofExceptionContext(EXCEPTION_POINTERS* info);
    bool ClearThreadContextSuspicion();
    bool RandomizeTiming();
    
    // Data
    std::atomic<bool> m_initialized{false};
    std::vector<VEHHandler> m_handlers;
    PVOID m_vehHandle{nullptr};
    
    // Thread-local exception nesting level (anti-detection)
    thread_local static int s_exceptionDepth;
};

/**
 * Memory Stealth Manager
 * Manages stealthy memory operations using VEH
 */
class StealthMemoryManager {
public:
    static StealthMemoryManager& Instance();
    
    // Stealth memory allocation
    PVOID StealthAlloc(SIZE_T size, DWORD protection = PAGE_EXECUTE_READWRITE);
    bool StealthFree(PVOID address);
    
    // Stealth memory access
    bool StealthRead(PVOID targetAddr, PVOID buffer, SIZE_T size);
    bool StealthWrite(PVOID targetAddr, PVOID data, SIZE_T size);
    
    // Code cave finding (for injection)
    PVOID FindCodeCave(HANDLE hProcess, SIZE_T size);
    
    // DLL stealth loading
    HMODULE StealthLoadLibrary(const char* dllPath);
    bool StealthUnloadLibrary(HMODULE hModule);
    
private:
    StealthMemoryManager() = default;
    
    // VEH-based read/write
    bool ReadViaException(HANDLE hProcess, PVOID addr, PVOID buffer, SIZE_T size);
    bool WriteViaException(HANDLE hProcess, PVOID addr, PVOID data, SIZE_T size);
};

/**
 * Anti-Cheat Evasion
 * Various techniques to evade anti-cheat detection
 */
class AntiCheatEvasion {
public:
    // Initialize all evasion techniques
    static bool InitializeFullEvasion();
    
    // Hide from common anti-cheats
    static bool HideFromRobloxAntiCheat();
    static bool HideFromByfron();
    static bool HideFromHyperion();
    
    // Process hiding
    static bool HideProcessFromDebugger();
    static bool UnlinkProcessFromPsLoadedModuleList();
    static bool RemoveProcessHeuristicFlags();
    
    // Memory hiding
    static bool HideMemoryRegion(PVOID base, SIZE_T size);
    static bool SpoofMemoryChecksums();
    
    // Thread hiding
    static bool HideThread(HANDLE hThread);
    static bool SpoofThreadStartAddress(HANDLE hThread);
    
private:
    // PEB manipulation
    static bool PatchPEBBeingDebugged();
    static bool PatchPEBNtGlobalFlag();
    static bool PatchPEBHeapFlags();
    
    // TEB manipulation
    static bool PatchTEBDebugInfo();
    
    // Kernel callbacks removal
    static bool RemoveThreadNotifications();
    static bool RemoveProcessNotifications();
    static bool RemoveImageLoadNotifications();
};

} // namespace NinjaEngine
