import net from 'net';
import { spawn } from 'child_process';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

/**
 * Ninja C++ Engine Bridge
 * Connects React UI to C++ DLL via named pipes
 */
class EngineBridge {
  constructor() {
    this.pipePath = '\\\\.\\pipe\\NinjaExecutor';
    this.client = null;
    this.connected = false;
    this.messageCallbacks = [];
    this.dllPath = path.join(__dirname, '..', 'cpp_engine', 'build', 'Release', 'ninja_engine.dll');
    this.injectorPath = path.join(__dirname, '..', 'cpp_engine', 'build', 'Release', 'NinjaInjector.exe');
  }

  // Check if DLL exists
  checkEngineFiles() {
    const dllExists = fs.existsSync(this.dllPath);
    const injectorExists = fs.existsSync(this.injectorPath);
    
    return {
      dll: dllExists,
      injector: injectorExists,
      dllPath: this.dllPath,
      injectorPath: this.injectorPath
    };
  }

  // Inject DLL into Roblox
  async injectEngine() {
    return new Promise((resolve, reject) => {
      const files = this.checkEngineFiles();
      
      if (!files.injector) {
        reject(new Error(`Injector not found: ${this.injectorPath}`));
        return;
      }
      
      if (!files.dll) {
        reject(new Error(`DLL not found: ${this.dllPath}`));
        return;
      }

      console.log('[Bridge] Injecting DLL into Roblox...');
      
      const injector = spawn(this.injectorPath, [this.dllPath], {
        detached: false,
        windowsHide: false
      });

      let output = '';
      let errorOutput = '';

      injector.stdout.on('data', (data) => {
        output += data.toString();
        console.log(`[Injector] ${data.toString().trim()}`);
      });

      injector.stderr.on('data', (data) => {
        errorOutput += data.toString();
        console.error(`[Injector Error] ${data.toString().trim()}`);
      });

      injector.on('close', (code) => {
        if (code === 0) {
          console.log('[Bridge] Injection successful!');
          resolve({ success: true, output });
        } else {
          reject(new Error(`Injection failed with code ${code}: ${errorOutput}`));
        }
      });

      injector.on('error', (err) => {
        reject(new Error(`Failed to start injector: ${err.message}`));
      });
    });
  }

  // Connect to C++ engine via named pipe
  async connect() {
    return new Promise((resolve, reject) => {
      if (this.connected) {
        resolve(true);
        return;
      }

      console.log('[Bridge] Connecting to C++ engine...');
      
      this.client = net.createConnection(this.pipePath, () => {
        console.log('[Bridge] Connected to C++ engine!');
        this.connected = true;
        resolve(true);
      });

      this.client.on('data', (data) => {
        const message = data.toString();
        console.log(`[Bridge] Received: ${message}`);
        
        // Notify all callbacks
        this.messageCallbacks.forEach(callback => callback(message));
      });

      this.client.on('end', () => {
        console.log('[Bridge] Disconnected from engine');
        this.connected = false;
      });

      this.client.on('error', (err) => {
        console.error(`[Bridge] Connection error: ${err.message}`);
        this.connected = false;
        
        // Check if pipe doesn't exist yet
        if (err.code === 'ENOENT') {
          reject(new Error('Engine not running. Please inject DLL first.'));
        } else {
          reject(err);
        }
      });

      // Timeout after 5 seconds
      setTimeout(() => {
        if (!this.connected) {
          reject(new Error('Connection timeout'));
        }
      }, 5000);
    });
  }

  // Disconnect from engine
  disconnect() {
    if (this.client) {
      this.client.end();
      this.client = null;
      this.connected = false;
      console.log('[Bridge] Disconnected');
    }
  }

  // Execute Lua script
  async executeScript(script) {
    return new Promise((resolve, reject) => {
      if (!this.connected) {
        reject(new Error('Not connected to engine'));
        return;
      }

      const request = JSON.stringify({
        type: 'execute',
        script: script,
        timestamp: Date.now()
      });

      console.log(`[Bridge] Executing script (${script.length} chars)`);

      // Set up one-time callback for this execution
      const handleResponse = (message) => {
        try {
          const response = JSON.parse(message);
          if (response.type === 'execution_result') {
            this.offMessage(handleResponse);
            resolve(response);
          }
        } catch (e) {
          // Not JSON or wrong type, ignore
        }
      };

      this.onMessage(handleResponse);

      // Send script to engine
      this.client.write(request + '\n');

      // Timeout after 30 seconds
      setTimeout(() => {
        this.offMessage(handleResponse);
        reject(new Error('Script execution timeout'));
      }, 30000);
    });
  }

  // Get engine status
  getStatus() {
    return {
      connected: this.connected,
      dllPath: this.dllPath,
      injectorPath: this.injectorPath,
      files: this.checkEngineFiles()
    };
  }

  // Register message callback
  onMessage(callback) {
    this.messageCallbacks.push(callback);
  }

  // Remove message callback
  offMessage(callback) {
    const index = this.messageCallbacks.indexOf(callback);
    if (index > -1) {
      this.messageCallbacks.splice(index, 1);
    }
  }
}

// Export for use in Electron
export { EngineBridge };

// If run directly, test connection
const isMainModule = import.meta.url === `file://${process.argv[1]}`;
if (isMainModule) {
  const bridge = new EngineBridge();
  
  console.log('Engine Bridge Test');
  console.log('==================');
  console.log('Status:', bridge.getStatus());
  
  // Try to connect (if engine is already running)
  bridge.connect()
    .then(() => {
      console.log('Connected!');
      
      // Test execute
      return bridge.executeScript('print("Hello from Node.js!")');
    })
    .then((result) => {
      console.log('Execution result:', result);
      bridge.disconnect();
    })
    .catch((err) => {
      console.error('Error:', err.message);
    });
}
