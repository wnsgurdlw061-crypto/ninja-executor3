#include <windows.h>
#include <iostream>
#include <string>
#include <tlhelp32.h>

// Simple DLL injector for Ninja Engine
// Run this as Administrator to inject into Roblox

bool EnableDebugPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);

    return result && GetLastError() == ERROR_SUCCESS;
}

DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(pe32);
        
        if (Process32FirstW(snapshot, &pe32)) {
            do {
                if (_wcsicmp(pe32.szExeFile, processName) == 0) {
                    processId = pe32.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &pe32));
        }
        
        CloseHandle(snapshot);
    }
    
    return processId;
}

bool InjectDLL(DWORD processId, const char* dllPath) {
    // Open target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Allocate memory for DLL path
    SIZE_T pathLen = strlen(dllPath) + 1;
    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        std::cerr << "Failed to allocate memory. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Write DLL path to remote process
    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, pathLen, NULL)) {
        std::cerr << "Failed to write memory. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Get LoadLibraryA address
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE loadLibraryAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");

    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryAddr, remoteMem, 0, NULL);
    if (!hThread) {
        std::cerr << "Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for injection to complete
    WaitForSingleObject(hThread, INFINITE);

    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "    Ninja Engine DLL Injector v1.0     " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Check for admin rights
    if (!EnableDebugPrivilege()) {
        std::cerr << "Warning: Could not enable debug privilege. Run as Administrator!" << std::endl;
    }

    // Find Roblox process
    std::cout << "[*] Searching for Roblox process..." << std::endl;
    DWORD robloxPid = GetProcessIdByName(L"RobloxPlayerBeta.exe");
    
    if (robloxPid == 0) {
        robloxPid = GetProcessIdByName(L"Windows10Universal.exe");
    }
    
    if (robloxPid == 0) {
        std::cerr << "[!] Roblox process not found!" << std::endl;
        std::cerr << "    Make sure Roblox is running." << std::endl;
        system("pause");
        return 1;
    }

    std::cout << "[+] Found Roblox process (PID: " << robloxPid << ")" << std::endl;

    // Get DLL path
    char dllPath[MAX_PATH];
    if (argc > 1) {
        strcpy_s(dllPath, argv[1]);
    } else {
        // Default path
        GetModuleFileNameA(NULL, dllPath, MAX_PATH);
        char* lastSlash = strrchr(dllPath, '\\');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';
        }
        strcat_s(dllPath, "ninja_engine.dll");
    }

    std::cout << "[*] DLL Path: " << dllPath << std::endl;

    // Check if DLL exists
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "[!] DLL not found: " << dllPath << std::endl;
        system("pause");
        return 1;
    }

    // Inject DLL
    std::cout << "[*] Injecting DLL..." << std::endl;
    if (InjectDLL(robloxPid, dllPath)) {
        std::cout << "[+] DLL injected successfully!" << std::endl;
        std::cout << std::endl;
        std::cout << "[*] You can now use the Ninja Executor UI." << std::endl;
    } else {
        std::cerr << "[!] Injection failed!" << std::endl;
        system("pause");
        return 1;
    }

    system("pause");
    return 0;
}
