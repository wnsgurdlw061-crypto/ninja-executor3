# VEH (Vector Exception Handling) Security Bypass

## Overview

VEH (Vector Exception Handling) is an advanced technique for bypassing anti-cheat security measures in games like Roblox. This implementation provides stealthy memory manipulation and anti-detection capabilities.

## What is VEH?

Vector Exception Handling is a Windows mechanism that allows applications to register handlers that receive exceptions before structured exception handling (SEH). This makes it powerful for:

- **Memory Access Control**: Intercept memory accesses before anti-cheat checks
- **Stealth Hooking**: Hook functions without modifying code
- **Protection Bypass**: Bypass PAGE_GUARD and PAGE_NOACCESS protections
- **Anti-Detection**: Hide from debuggers and anti-cheat systems

## How It Works

```
Normal Execution:
Code → Memory Access → Anti-Cheat Check → Continue

VEH Bypass:
Code → Memory Access → VEH Handler → Our Code → Anti-Cheat Bypassed → Continue
```

## Features

### 1. Guard Page Hooks
```cpp
// Set up guard page to trigger on memory access
VEHSecurityBypass::Instance().AddGuardPageHook(
    targetAddress,     // Address to monitor
    size,              // Size of region
    callback           // Called when accessed
);
```

### 2. Stealth Breakpoints
```cpp
// Set INT3 breakpoint without modifying code permanently
VEHSecurityBypass::Instance().AddBreakpointHook(
    functionAddress,   // Function to hook
    callback           // Called on breakpoint
);
```

### 3. Memory Protection Bypass
```cpp
// Read/write protected memory stealthily
StealthMemoryManager::Instance().StealthRead(target, buffer, size);
StealthMemoryManager::Instance().StealthWrite(target, data, size);
```

### 4. Anti-Cheat Evasion
```cpp
// Initialize full evasion
AntiCheatEvasion::InitializeFullEvasion();

// Or individual techniques:
AntiCheatEvasion::HideFromRobloxAntiCheat();
AntiCheatEvasion::HideFromByfron();
AntiCheatEvasion::HideFromHyperion();
```

## Usage Example

```cpp
#include "veh_security.h"

using namespace NinjaEngine;

// Initialize VEH system
bool InitializeVEH() {
    // Initialize VEH handler
    if (!VEHSecurityBypass::Instance().Initialize()) {
        return false;
    }
    
    // Initialize anti-cheat evasion
    if (!AntiCheatEvasion::InitializeFullEvasion()) {
        return false;
    }
    
    // Set up guard page hook on Lua function
    PVOID luaFunction = FindLuaFunction();
    VEHSecurityBypass::Instance().AddGuardPageHook(
        luaFunction,
        0x1000,
        [](EXCEPTION_POINTERS* info) -> bool {
            // Called when Lua function is accessed
            // Modify execution, bypass checks, etc.
            return true; // Continue execution
        }
    );
    
    return true;
}

// Stealth memory operations
void StealthOperations() {
    // Read protected memory
    char buffer[256];
    StealthMemoryManager::Instance().StealthRead(
        (PVOID)0x7FF123456000,
        buffer,
        sizeof(buffer)
    );
    
    // Write to protected memory
    BYTE patch[] = { 0x90, 0x90, 0x90 }; // NOPs
    StealthMemoryManager::Instance().StealthWrite(
        (PVOID)0x7FF123456000,
        patch,
        sizeof(patch)
    );
}
```

## Anti-Cheat Bypass Techniques

### 1. PEB Patching
- **BeingDebugged**: Clears debugger flag
- **NtGlobalFlag**: Removes debug heap flags
- **HeapFlags**: Patches heap validation flags

### 2. TEB Manipulation
- Clears debug register information
- Removes thread debug context

### 3. Hardware Breakpoints
- Clears DR0-DR3 registers
- Disables debug registers

### 4. Process Hiding
- Unlinks from loaded module list
- Hides from process enumeration

## Security Considerations

⚠️ **WARNING**: This tool is for educational purposes only.

- Uses advanced memory manipulation techniques
- May trigger some anti-cheat systems
- Use at your own risk
- Violates most online game Terms of Service

## Technical Details

### VEH Handler Chain
```
Exception Occurs
    ↓
VEH Handler (First)
    ↓
Our Handler Processes
    ↓
EXCEPTION_CONTINUE_EXECUTION (Bypass)
    ↓
OR
    ↓
EXCEPTION_CONTINUE_SEARCH (Pass to next)
```

### Exception Types Handled

| Exception | Use Case |
|-----------|----------|
| EXCEPTION_GUARD_PAGE | Memory access monitoring |
| EXCEPTION_BREAKPOINT | Code execution hooks |
| EXCEPTION_SINGLE_STEP | Instruction tracing |
| EXCEPTION_ACCESS_VIOLATION | Protected memory access |

### Anti-Detection Measures

1. **Exception Depth Tracking**: Prevents nested exception detection
2. **Timing Randomization**: Adds random delays to avoid pattern detection
3. **Context Spoofing**: Modifies exception context to hide artifacts
4. **Hardware Register Clearing**: Clears debug registers

## Building with VEH

The VEH system is automatically included when building the engine:

```batch
cd cpp_engine
build.bat
```

Files included:
- `veh_security.h` - Header file
- `veh_security.cpp` - Implementation

## Troubleshooting

### VEH Handler Not Called
- Ensure `Initialize()` is called first
- Check if anti-cheat blocks VEH registration
- Try running with elevated privileges

### Memory Access Still Blocked
- Use `StealthMemoryManager` instead of direct access
- Check if memory is truly allocated
- Verify address is in valid range

### Anti-Cheat Detects Injection
- Enable all evasion techniques
- Use `HideFromByfron()` or `HideFromHyperion()`
- Clear hardware breakpoints

## Integration with C# UI

The C# UI automatically uses VEH when available:

```csharp
// In MainWindow.xaml.cs
private async void InjectButton_Click(...) {
    // VEH mode is used automatically by the C++ engine
    bool success = await InjectDLL(...);
    
    if (success) {
        Log("Injected with VEH bypass enabled");
    }
}
```

## References

- [Microsoft VEH Documentation](https://docs.microsoft.com/en-us/windows/win32/debug/vectored-exception-handling)
- [Windows Internals](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/)
- [Roblox Anti-Cheat Research](https://devforum.roblox.com/)

## License

Educational use only. See main LICENSE file.
