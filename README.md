# Ninja C++ DLL Engine

## Overview

This is the C++ DLL execution engine for the Ninja Roblox Executor. It provides:

- **DLL Injection**: Injects into Roblox process
- **Lua Script Execution**: Executes Lua scripts in Roblox context
- **Memory Management**: Read/write Roblox process memory
- **IPC Communication**: Pipe-based communication with UI

## Architecture

```
┌─────────────┐      ┌──────────────┐      ┌─────────────┐
│  React UI   │◄────►│  Node.js     │◄────►│  C++ DLL    │
│  (Electron) │      │  Bridge      │      │  (Injected) │
└─────────────┘      └──────────────┘      └──────┬──────┘
                                                  │
                                                  ▼
                                           ┌─────────────┐
                                           │  Roblox     │
                                           │  Process    │
                                           └─────────────┘
```

## Building

### Prerequisites

- Visual Studio 2022 (with C++ support)
- CMake 3.16+
- Windows SDK

### Build Steps

1. Open terminal in `cpp_engine` folder
2. Run build script:
   ```batch
   build.bat
   ```

Or manually:
```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Output: `build/Release/ninja_engine.dll`

## API Reference

### DLL Exports

```cpp
// Initialize the engine
bool InitializeEngine();

// Shutdown the engine
void ShutdownEngine();

// Execute Lua script
bool ExecuteScript(const char* script, char* output, size_t outputSize);

// Check if engine is ready
bool IsEngineReady();

// Get engine version
const char* GetEngineVersion();
```

### Usage Example

```cpp
#include <windows.h>
#include <iostream>

// Load DLL
HMODULE hDll = LoadLibraryA("ninja_engine.dll");

// Get function pointers
typedef bool (*InitializeEngine_t)();
typedef bool (*ExecuteScript_t)(const char*, char*, size_t);

InitializeEngine_t Init = (InitializeEngine_t)GetProcAddress(hDll, "InitializeEngine");
ExecuteScript_t Execute = (ExecuteScript_t)GetProcAddress(hDll, "ExecuteScript");

// Initialize
if (Init()) {
    char output[4096];
    if (Execute("print('Hello Roblox!')", output, sizeof(output))) {
        std::cout << "Output: " << output << std::endl;
    }
}
```

## Components

### 1. MemoryManager

Handles Roblox process memory operations:

- `AttachToProcess(pid)` - Attach to Roblox
- `ReadMemory(addr, buffer, size)` - Read process memory
- `WriteMemory(addr, data, size)` - Write process memory
- `AllocateMemory(size)` - Allocate memory in target process
- `GetRobloxProcessId()` - Find Roblox process

### 2. ExecutionEngine

Main script execution engine:

- `Initialize()` - Initialize Lua context
- `ExecuteScript(script)` - Execute Lua code
- `InjectIntoRoblox()` - Inject DLL into Roblox
- `SetMessageCallback(fn)` - Set output callback

### 3. CommunicationPipe

IPC for UI communication:

- `Create(pipeName)` - Create named pipe
- `SendMessage(msg)` - Send message to UI
- `ReceiveMessage()` - Receive message from UI

## Security Notes

⚠️ **Warning**: This tool is for educational purposes only.

- Uses process injection techniques
- Modifies game memory
- May trigger anti-cheat systems
- Use at your own risk

## Integration with Electron

The Node.js bridge (`cpp_engine/server.js`) connects the React UI with the C++ DLL:

```javascript
const { spawn } = require('child_process');
const net = require('net');

// Connect to DLL pipe
const client = net.createConnection('\\\\.\\pipe\\NinjaExecutor');

// Send script
client.write(JSON.stringify({
  type: 'execute',
  script: 'print("Hello")'
}));

// Receive output
client.on('data', (data) => {
  console.log('Output:', data.toString());
});
```

## Troubleshooting

### Build Errors

1. **"Cannot find windows.h"** - Install Windows SDK
2. **"CMake not found"** - Install CMake and add to PATH
3. **"Visual Studio not found"** - Install VS 2022 with C++ workload

### Runtime Errors

1. **"DLL injection failed"** - Run as Administrator
2. **"Pipe connection failed"** - Check antivirus/firewall
3. **"Roblox not found"** - Start Roblox before injecting

## License

Educational use only. See LICENSE for details.
