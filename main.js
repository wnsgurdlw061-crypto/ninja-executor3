const { app, BrowserWindow, ipcMain, dialog, shell } = require('electron');
const path = require('path');
const fs = require('fs');

let mainWindow;
let engineProcess = null;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1400,
        height: 900,
        minWidth: 1200,
        minHeight: 700,
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            enableRemoteModule: false,
            preload: path.join(__dirname, 'preload.js')
        },
        icon: path.join(__dirname, 'icon.ico'),
        show: false,
        frame: false,
        transparent: false,
        backgroundColor: '#0f172a',
        webSecurity: true
    });

    mainWindow.loadFile('index.html');
    
    mainWindow.once('ready-to-show', () => {
        mainWindow.show();
        mainWindow.center();
    });

    mainWindow.on('closed', () => {
        mainWindow = null;
        if (engineProcess) {
            engineProcess.kill();
            engineProcess = null;
        }
    });

    // Open DevTools in development
    if (process.env.NODE_ENV === 'development') {
        mainWindow.webContents.openDevTools();
    }
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
        createWindow();
    }
});

// IPC Handlers
ipcMain.handle('minimize-window', () => {
    if (mainWindow) {
        mainWindow.minimize();
    }
});

ipcMain.handle('maximize-window', () => {
    if (mainWindow) {
        if (mainWindow.isMaximized()) {
            mainWindow.unmaximize();
        } else {
            mainWindow.maximize();
        }
    }
});

ipcMain.handle('close-window', () => {
    if (mainWindow) {
        mainWindow.close();
    }
});

// Engine Functions
ipcMain.handle('inject-engine', async () => {
    try {
        // Simulate engine injection
        return { success: true, message: 'Engine injected successfully!' };
    } catch (error) {
        return { success: false, message: error.message };
    }
});

ipcMain.handle('execute-script', async (event, script) => {
    try {
        // Simulate script execution
        const result = {
            success: true,
            output: `Script executed: ${script.substring(0, 50)}...`,
            executionTime: Math.floor(Math.random() * 1000) + 100
        };
        return result;
    } catch (error) {
        return { success: false, message: error.message };
    }
});

ipcMain.handle('stop-execution', async () => {
    try {
        return { success: true, message: 'Execution stopped' };
    } catch (error) {
        return { success: false, message: error.message };
    }
});

// File Operations
ipcMain.handle('save-script', async (event, script, filename) => {
    try {
        const result = await dialog.showSaveDialog(mainWindow, {
            defaultPath: filename,
            filters: [
                { name: 'Lua Files', extensions: ['lua'] },
                { name: 'All Files', extensions: ['*'] }
            ]
        });

        if (result.canceled) return { success: false, message: 'Save cancelled' };

        fs.writeFileSync(result.filePath, script, 'utf8');
        return { success: true, message: 'Script saved successfully' };
    } catch (error) {
        return { success: false, message: error.message };
    }
});

ipcMain.handle('load-script', async () => {
    try {
        const result = await dialog.showOpenDialog(mainWindow, {
            filters: [
                { name: 'Lua Files', extensions: ['lua'] },
                { name: 'All Files', extensions: ['*'] }
            ],
            properties: ['openFile']
        });

        if (result.canceled) return { success: false, message: 'Load cancelled' };

        const script = fs.readFileSync(result.filePaths[0], 'utf8');
        return { success: true, script };
    } catch (error) {
        return { success: false, message: error.message };
    }
});

// Prevent multiple instances
const gotTheLock = app.requestSingleInstanceLock();

if (!gotTheLock) {
    app.quit();
} else {
    app.on('second-instance', () => {
        if (mainWindow) {
            if (mainWindow.isMinimized()) mainWindow.restore();
            mainWindow.focus();
        }
    });
}
