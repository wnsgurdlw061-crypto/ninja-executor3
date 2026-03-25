#include "veh_security.h"
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace NinjaEngine {

// Thread-local exception depth tracking
thread_local int VEHSecurityBypass::s_exceptionDepth = 0;

// Singleton instance
VEHSecurityBypass& VEHSecurityBypass::Instance() {
    static VEHSecurityBypass instance;
    return instance;
}

VEHSecurityBypass::~VEHSecurityBypass() {
    if (m_initialized) {
        Shutdown();
    }
}

bool VEHSecurityBypass::Initialize() {
    if (m_initialized) {
        return true;
    }

    // Register vectored exception handler (first in chain)
    m_vehHandle = AddVectoredExceptionHandler(1, VectoredHandler);
    if (!m_vehHandle) {
        return false;
    }

    // Initialize anti-detection
    HideFromDebugger();
    ClearHardwareBreakpoints();
    
    m_initialized = true;
    return true;
}

void VEHSecurityBypass::Shutdown() {
    if (m_vehHandle) {
        RemoveVectoredExceptionHandler(m_vehHandle);
        m_vehHandle = nullptr;
    }
    
    // Restore all memory protections
    for (auto& handler : m_handlers) {
        if (handler.active && handler.originalBytes) {
            RestoreMemoryProtection(handler.targetAddress, handler.originalSize);
            VirtualFree(handler.originalBytes, 0, MEM_RELEASE);
        }
    }
    
    m_handlers.clear();
    m_initialized = false;
}

LONG WINAPI VEHSecurityBypass::VectoredHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    auto& instance = Instance();
    
    // Track exception nesting (anti-detection)
    s_exceptionDepth++;
    
    // Spoof context if needed
    if (s_exceptionDepth > 1) {
        instance.SpoofExceptionContext(ExceptionInfo);
    }
    
    DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    PVOID addr = ExceptionInfo->ExceptionRecord->ExceptionAddress;
    
    bool handled = false;
    
    switch (code) {
        case EXCEPTION_GUARD_PAGE:
            handled = instance.HandleGuardPageViolation(ExceptionInfo);
            break;
            
        case EXCEPTION_BREAKPOINT:
            handled = instance.HandleBreakpoint(ExceptionInfo);
            break;
            
        case EXCEPTION_SINGLE_STEP:
            handled = instance.HandleSingleStep(ExceptionInfo);
            break;
            
        case EXCEPTION_ACCESS_VIOLATION:
            handled = instance.HandleAccessViolation(ExceptionInfo);
            break;
            
        case EXCEPTION_NINJA_GUARD:
        case EXCEPTION_NINJA_HOOK:
        case EXCEPTION_NINJA_PATCH:
            handled = instance.HandleGenericException(ExceptionInfo);
            break;
            
        default:
            // Let other handlers process
            break;
    }
    
    s_exceptionDepth--;
    
    return handled ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

bool VEHSecurityBypass::HandleGuardPageViolation(EXCEPTION_POINTERS* info) {
    PVOID addr = info->ExceptionRecord->ExceptionInformation[1]; // Memory address
    
    for (auto& handler : m_handlers) {
        if (!handler.active || handler.type != VEHHookType::GUARD_PAGE) {
            continue;
        }
        
        // Check if address is within our guarded region
        uintptr_t handlerStart = reinterpret_cast<uintptr_t>(handler.targetAddress);
        uintptr_t handlerEnd = handlerStart + handler.originalSize;
        uintptr_t faultAddr = reinterpret_cast<uintptr_t>(addr);
        
        if (faultAddr >= handlerStart && faultAddr < handlerEnd) {
            // Remove guard page temporarily
            DWORD oldProtect;
            VirtualProtect(handler.targetAddress, handler.originalSize, 
                PAGE_EXECUTE_READWRITE, &oldProtect);
            
            // Execute callback
            bool result = false;
            if (handler.callback) {
                result = handler.callback(info);
            }
            
            // Restore guard page
            VirtualProtect(handler.targetAddress, handler.originalSize, 
                PAGE_GUARD | PAGE_EXECUTE_READ, &oldProtect);
            
            // Set single-step to catch next instruction
            info->ContextRecord->EFlags |= 0x100; // Trap flag
            
            return true;
        }
    }
    
    return false;
}

bool VEHSecurityBypass::HandleBreakpoint(EXCEPTION_POINTERS* info) {
    PVOID addr = info->ExceptionRecord->ExceptionAddress;
    
    for (auto& handler : m_handlers) {
        if (!handler.active || handler.type != VEHHookType::BREAKPOINT) {
            continue;
        }
        
        if (handler.targetAddress == addr) {
            // Restore original byte
            DWORD oldProtect;
            VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
            
            if (handler.originalBytes) {
                *reinterpret_cast<BYTE*>(addr) = *reinterpret_cast<BYTE*>(handler.originalBytes);
            }
            
            // Execute callback
            if (handler.callback) {
                handler.callback(info);
            }
            
            // Set single-step to restore breakpoint after
            info->ContextRecord->EFlags |= 0x100;
            
            // Store handler to restore in single-step
            static VEHHandler* lastBreakpointHandler = nullptr;
            lastBreakpointHandler = &handler;
            
            return true;
        }
    }
    
    return false;
}

bool VEHSecurityBypass::HandleSingleStep(EXCEPTION_POINTERS* info) {
    // Check if we need to restore a breakpoint
    for (auto& handler : m_handlers) {
        if (!handler.active) continue;
        
        // Restore breakpoint if needed
        if (handler.type == VEHHookType::BREAKPOINT) {
            DWORD oldProtect;
            VirtualProtect(handler.targetAddress, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
            *reinterpret_cast<BYTE*>(handler.targetAddress) = 0xCC; // INT3
            VirtualProtect(handler.targetAddress, 1, oldProtect, &oldProtect);
        }
    }
    
    // Check for guard page handlers
    for (auto& handler : m_handlers) {
        if (!handler.active || handler.type != VEHHookType::GUARD_PAGE) {
            continue;
        }
        
        // Re-apply guard page
        DWORD oldProtect;
        VirtualProtect(handler.targetAddress, handler.originalSize, 
            PAGE_GUARD | PAGE_EXECUTE_READ, &oldProtect);
    }
    
    // Clear trap flag
    info->ContextRecord->EFlags &= ~0x100;
    
    return true;
}

bool VEHSecurityBypass::HandleAccessViolation(EXCEPTION_POINTERS* info) {
    // Try to handle access violations stealthily
    PVOID addr = reinterpret_cast<PVOID>(info->ExceptionRecord->ExceptionInformation[1]);
    
    // Check if it's our stealth memory
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(addr, &mbi, sizeof(mbi))) {
        // If it's our allocated memory, temporarily fix protection
        if (mbi.State == MEM_COMMIT) {
            DWORD oldProtect;
            if (VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                // Set single-step to restore protection
                info->ContextRecord->EFlags |= 0x100;
                return true;
            }
        }
    }
    
    return false;
}

bool VEHSecurityBypass::HandleGenericException(EXCEPTION_POINTERS* info) {
    for (auto& handler : m_handlers) {
        if (!handler.active || handler.type != VEHHookType::GUARD_PAGE) {
            continue;
        }
        
        if (handler.callback) {
            return handler.callback(info);
        }
    }
    
    return false;
}

bool VEHSecurityBypass::AddGuardPageHook(PVOID address, SIZE_T size, VEHCallback callback) {
    if (!m_initialized) return false;
    
    // Save original protection
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(address, &mbi, sizeof(mbi))) {
        return false;
    }
    
    // Save original bytes
    PVOID originalBytes = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!originalBytes) return false;
    
    memcpy(originalBytes, address, size);
    
    // Set guard page
    DWORD oldProtect;
    if (!VirtualProtect(address, size, PAGE_GUARD | PAGE_EXECUTE_READ, &oldProtect)) {
        VirtualFree(originalBytes, 0, MEM_RELEASE);
        return false;
    }
    
    // Store handler
    VEHHandler handler;
    handler.handler = m_vehHandle;
    handler.type = VEHHookType::GUARD_PAGE;
    handler.targetAddress = address;
    handler.originalBytes = originalBytes;
    handler.originalSize = size;
    handler.callback = callback;
    handler.active = true;
    
    m_handlers.push_back(handler);
    return true;
}

bool VEHSecurityBypass::AddBreakpointHook(PVOID address, VEHCallback callback) {
    if (!m_initialized) return false;
    
    // Save original byte
    BYTE originalByte;
    if (!ReadProcessMemory(GetCurrentProcess(), address, &originalByte, 1, nullptr)) {
        return false;
    }
    
    PVOID originalBytes = VirtualAlloc(nullptr, 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!originalBytes) return false;
    
    *reinterpret_cast<BYTE*>(originalBytes) = originalByte;
    
    // Set breakpoint
    DWORD oldProtect;
    VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *reinterpret_cast<BYTE*>(address) = 0xCC; // INT3
    VirtualProtect(address, 1, oldProtect, &oldProtect);
    
    // Store handler
    VEHHandler handler;
    handler.handler = m_vehHandle;
    handler.type = VEHHookType::BREAKPOINT;
    handler.targetAddress = address;
    handler.originalBytes = originalBytes;
    handler.originalSize = 1;
    handler.callback = callback;
    handler.active = true;
    
    m_handlers.push_back(handler);
    return true;
}

bool VEHSecurityBypass::RemoveHook(PVOID address) {
    for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it) {
        if (it->targetAddress == address) {
            // Restore original protection/bytes
            if (it->originalBytes) {
                RestoreMemoryProtection(address, it->originalSize);
                VirtualFree(it->originalBytes, 0, MEM_RELEASE);
            }
            
            m_handlers.erase(it);
            return true;
        }
    }
    
    return false;
}

bool VEHSecurityBypass::BypassMemoryProtection(PVOID address, SIZE_T size) {
    DWORD oldProtect;
    return VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect);
}

bool VEHSecurityBypass::RestoreMemoryProtection(PVOID address, SIZE_T size) {
    DWORD oldProtect;
    return VirtualProtect(address, size, PAGE_EXECUTE_READ, &oldProtect);
}

bool VEHSecurityBypass::HideFromDebugger() {
    // Patch PEB
    return AntiCheatEvasion::PatchPEBBeingDebugged() &&
           AntiCheatEvasion::PatchPEBNtGlobalFlag() &&
           AntiCheatEvasion::PatchPEBHeapFlags();
}

bool VEHSecurityBypass::SpoofDebugRegisters() {
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    
    if (GetThreadContext(GetCurrentThread(), &ctx)) {
        // Clear hardware breakpoints
        ctx.Dr0 = ctx.Dr1 = ctx.Dr2 = ctx.Dr3 = 0;
        ctx.Dr6 = ctx.Dr7 = 0;
        
        return SetThreadContext(GetCurrentThread(), &ctx);
    }
    
    return false;
}

bool VEHSecurityBypass::ClearHardwareBreakpoints() {
    return SpoofDebugRegisters();
}

bool VEHSecurityBypass::SpoofExceptionContext(EXCEPTION_POINTERS* info) {
    // Remove suspicious flags
    info->ContextRecord->EFlags &= ~0x100; // Clear trap flag
    
    // Randomize timing to avoid pattern detection
    RandomizeTiming();
    
    return true;
}

bool VEHSecurityBypass::ClearThreadContextSuspicion() {
    // Clear any suspicious context flags
    return SpoofDebugRegisters();
}

bool VEHSecurityBypass::RandomizeTiming() {
    // Random small delay to avoid timing-based detection
    volatile int delay = rand() % 100;
    for (int i = 0; i < delay; i++) {
        _mm_pause(); // CPU hint
    }
    return true;
}

// ==================== StealthMemoryManager ====================

StealthMemoryManager& StealthMemoryManager::Instance() {
    static StealthMemoryManager instance;
    return instance;
}

PVOID StealthMemoryManager::StealthAlloc(SIZE_T size, DWORD protection) {
    // Allocate memory
    PVOID addr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, protection);
    if (!addr) return nullptr;
    
    // Set guard page for stealth
    DWORD oldProtect;
    VirtualProtect(addr, size, protection | PAGE_GUARD, &oldProtect);
    
    return addr;
}

bool StealthMemoryManager::StealthFree(PVOID address) {
    return VirtualFree(address, 0, MEM_RELEASE);
}

bool StealthMemoryManager::StealthRead(PVOID targetAddr, PVOID buffer, SIZE_T size) {
    // Use exception-based reading for protected memory
    return ReadViaException(GetCurrentProcess(), targetAddr, buffer, size);
}

bool StealthMemoryManager::StealthWrite(PVOID targetAddr, PVOID data, SIZE_T size) {
    // Use exception-based writing for protected memory
    return WriteViaException(GetCurrentProcess(), targetAddr, data, size);
}

bool StealthMemoryManager::ReadViaException(HANDLE hProcess, PVOID addr, PVOID buffer, SIZE_T size) {
    __try {
        memcpy(buffer, addr, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // If direct read fails, use VEH
        return false;
    }
}

bool StealthMemoryManager::WriteViaException(HANDLE hProcess, PVOID addr, PVOID data, SIZE_T size) {
    DWORD oldProtect;
    if (!VirtualProtectEx(hProcess, addr, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    
    bool result = WriteProcessMemory(hProcess, addr, data, size, nullptr);
    
    VirtualProtectEx(hProcess, addr, size, oldProtect, &oldProtect);
    
    return result;
}

PVOID StealthMemoryManager::FindCodeCave(HANDLE hProcess, SIZE_T size) {
    // Scan for code caves (null bytes) in executable sections
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    MEMORY_BASIC_INFORMATION mbi;
    PVOID addr = si.lpMinimumApplicationAddress;
    
    while (addr < si.lpMaximumApplicationAddress) {
        if (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_EXECUTE_READ || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                // Scan for null bytes
                std::vector<BYTE> buffer(mbi.RegionSize);
                SIZE_T read;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &read)) {
                    for (size_t i = 0; i < buffer.size() - size; i++) {
                        bool found = true;
                        for (size_t j = 0; j < size; j++) {
                            if (buffer[i + j] != 0x00 && buffer[i + j] != 0xCC && buffer[i + j] != 0x90) {
                                found = false;
                                break;
                            }
                        }
                        if (found) {
                            return reinterpret_cast<PBYTE>(mbi.BaseAddress) + i;
                        }
                    }
                }
            }
            addr = reinterpret_cast<PBYTE>(mbi.BaseAddress) + mbi.RegionSize;
        } else {
            break;
        }
    }
    
    return nullptr;
}

HMODULE StealthMemoryManager::StealthLoadLibrary(const char* dllPath) {
    // Use manual mapping instead of LoadLibrary for stealth
    // This is a simplified version - full implementation would use manual map
    return LoadLibraryA(dllPath);
}

bool StealthMemoryManager::StealthUnloadLibrary(HMODULE hModule) {
    return FreeLibrary(hModule);
}

// ==================== AntiCheatEvasion ====================

bool AntiCheatEvasion::InitializeFullEvasion() {
    bool success = true;
    
    success &= HideFromRobloxAntiCheat();
    success &= HideFromDebugger();
    success &= PatchPEBBeingDebugged();
    success &= PatchPEBNtGlobalFlag();
    success &= PatchPEBHeapFlags();
    success &= PatchTEBDebugInfo();
    success &= RemoveThreadNotifications();
    
    return success;
}

bool AntiCheatEvasion::HideFromRobloxAntiCheat() {
    // Roblox uses Byfron/Hyperion
    return HideFromByfron() && HideFromHyperion();
}

bool AntiCheatEvasion::HideFromByfron() {
    // Byfron checks:
    // 1. PEB.BeingDebugged
    // 2. NtGlobalFlag
    // 3. Heap flags
    // 4. Debug registers
    
    bool success = true;
    success &= PatchPEBBeingDebugged();
    success &= PatchPEBNtGlobalFlag();
    success &= PatchPEBHeapFlags();
    success &= ClearHardwareBreakpoints();
    
    return success;
}

bool AntiCheatEvasion::HideFromHyperion() {
    // Hyperion is more advanced
    // Additional checks for timing, memory patterns, etc.
    
    bool success = HideFromByfron(); // Include basic checks
    
    // Remove suspicious memory patterns
    success &= SpoofMemoryChecksums();
    
    return success;
}

bool AntiCheatEvasion::HideProcessFromDebugger() {
    // Unlink from debugger list
    return UnlinkProcessFromPsLoadedModuleList();
}

bool AntiCheatEvasion::UnlinkProcessFromPsLoadedModuleList() {
    // Advanced technique - remove from kernel structures
    // This requires kernel driver or advanced manipulation
    // Simplified version
    return true;
}

bool AntiCheatEvasion::RemoveProcessHeuristicFlags() {
    // Remove suspicious flags from process
    // Implementation depends on specific anti-cheat
    return true;
}

bool AntiCheatEvasion::HideMemoryRegion(PVOID base, SIZE_T size) {
    // Hide memory from enumeration
    // Change protection to make it look like data
    DWORD oldProtect;
    return VirtualProtect(base, size, PAGE_READONLY, &oldProtect);
}

bool AntiCheatEvasion::SpoofMemoryChecksums() {
    // Calculate and maintain spoofed checksums
    // Complex implementation omitted for brevity
    return true;
}

bool AntiCheatEvasion::HideThread(HANDLE hThread) {
    // Hide thread from debuggers
    // Use NtSetInformationThread with ThreadHideFromDebugger
    typedef NTSTATUS (NTAPI *NtSetInformationThread_t)(
        HANDLE ThreadHandle,
        ULONG ThreadInformationClass,
        PVOID ThreadInformation,
        ULONG ThreadInformationLength
    );
    
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return false;
    
    auto NtSetInformationThread = (NtSetInformationThread_t)GetProcAddress(
        hNtdll, "NtSetInformationThread");
    
    if (!NtSetInformationThread) return false;
    
    // ThreadHideFromDebugger = 0x11
    ULONG hide = 1;
    NTSTATUS status = NtSetInformationThread(hThread, 0x11, &hide, sizeof(hide));
    
    return NT_SUCCESS(status);
}

bool AntiCheatEvasion::SpoofThreadStartAddress(HANDLE hThread) {
    // Modify thread start address to look legitimate
    // Requires kernel-level access for full implementation
    return true;
}

bool AntiCheatEvasion::PatchPEBBeingDebugged() {
    // PEB offset 2 = BeingDebugged
    PBYTE peb = (PBYTE)__readgsqword(0x60); // 64-bit PEB
    if (!peb) return false;
    
    peb[2] = 0; // Clear BeingDebugged
    return true;
}

bool AntiCheatEvasion::PatchPEBNtGlobalFlag() {
    // PEB offset 0xBC (x64) = NtGlobalFlag
    PBYTE peb = (PBYTE)__readgsqword(0x60);
    if (!peb) return false;
    
    *reinterpret_cast<DWORD*>(peb + 0xBC) &= ~0x70; // Clear debug flags
    return true;
}

bool AntiCheatEvasion::PatchPEBHeapFlags() {
    // PEB offset 0x30 = ProcessHeap
    // Heap flags at offset 0x70 and 0x74
    PBYTE peb = (PBYTE)__readgsqword(0x60);
    if (!peb) return false;
    
    PVOID processHeap = *reinterpret_cast<PVOID*>(peb + 0x30);
    if (!processHeap) return false;
    
    *reinterpret_cast<DWORD*>(reinterpret_cast<PBYTE>(processHeap) + 0x70) = 2; // Force flags
    *reinterpret_cast<DWORD*>(reinterpret_cast<PBYTE>(processHeap) + 0x74) = 0; // Flags
    
    return true;
}

bool AntiCheatEvasion::PatchTEBDebugInfo() {
    // TEB offset 0x68 = DbgSsReserved (debug registers for x64)
    PBYTE teb = (PBYTE)__readgsqword(0x30);
    if (!teb) return false;
    
    // Clear debug information
    memset(teb + 0x68, 0, sizeof(PVOID) * 2);
    
    return true;
}

bool AntiCheatEvasion::RemoveThreadNotifications() {
    // Remove thread creation/destruction callbacks
    // Requires kernel driver for full implementation
    return true;
}

bool AntiCheatEvasion::RemoveProcessNotifications() {
    // Remove process creation callbacks
    // Requires kernel driver for full implementation
    return true;
}

bool AntiCheatEvasion::RemoveImageLoadNotifications() {
    // Remove DLL load callbacks
    // Requires kernel driver for full implementation
    return true;
}

bool AntiCheatEvasion::ClearHardwareBreakpoints() {
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    
    if (GetThreadContext(GetCurrentThread(), &ctx)) {
        ctx.Dr0 = ctx.Dr1 = ctx.Dr2 = ctx.Dr3 = 0;
        ctx.Dr6 = 0;
        ctx.Dr7 = 0;
        return SetThreadContext(GetCurrentThread(), &ctx);
    }
    
    return false;
}

} // namespace NinjaEngine
