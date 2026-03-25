const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods to renderer process
contextBridge.exposeInMainWorld('electronAPI', {
    // Window controls
    minimizeWindow: () => ipcRenderer.invoke('minimize-window'),
    maximizeWindow: () => ipcRenderer.invoke('maximize-window'),
    closeWindow: () => ipcRenderer.invoke('close-window'),
    
    // Engine operations
    injectEngine: () => ipcRenderer.invoke('inject-engine'),
    executeScript: (script) => ipcRenderer.invoke('execute-script', script),
    stopExecution: () => ipcRenderer.invoke('stop-execution'),
    
    // File operations
    saveScript: (script, filename) => ipcRenderer.invoke('save-script', script, filename),
    loadScript: () => ipcRenderer.invoke('load-script'),
    
    // System info
    platform: process.platform,
    version: process.versions.electron
});
