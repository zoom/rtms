import { execSync } from 'child_process';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';
import fs from 'fs';

// Path utilities
export function getProjectRoot() {
  const __filename = fileURLToPath(import.meta.url);
  const __dirname = dirname(__filename);
  return join(__dirname, '..', '..');
}

// Get build configuration (debug/release)
export function getBuildMode() {
  // Check for NODE_ENV, RTMS_BUILD_MODE, or default to 'release'
  const mode = (process.env.RTMS_BUILD_MODE || process.env.NODE_ENV || 'release').toLowerCase();
  return mode === 'development' || mode === 'debug' ? 'debug' : 'release';
}

// Logging utilities with colors
export function log(prefix, message) {
  console.log(`\x1b[36m[RTMS ${prefix}]\x1b[0m ${message}`);
}

export function error(prefix, message) {
  console.error(`\x1b[31m[RTMS ${prefix} Error]\x1b[0m ${message}`);
  process.exit(1);
}

export function success(prefix, message) {
  console.log(`\x1b[32m[RTMS ${prefix} Success]\x1b[0m ${message}`);
}

export function warning(prefix, message) {
  console.log(`\x1b[33m[RTMS ${prefix} Warning]\x1b[0m ${message}`);
}

export function run(command, prefix, options = {}) {
  try {
    // Get the current build mode
    const buildMode = getBuildMode();
    
    // Inject build mode into environment variables for child processes
    const env = { 
      ...process.env, 
      RTMS_BUILD_MODE: buildMode,
      ...options.env 
    };
    
    log(prefix, `Running [${buildMode}]: ${command}`);
    
    execSync(command, { 
      stdio: 'inherit', 
      cwd: getProjectRoot(),
      env,
      ...options 
    });
    return true;
  } catch (err) {
    error(prefix, `Command failed: ${command}\n${err.message}`);
    return false;
  }
}

export function removeDir(dirPath, prefix = "Clean") {
  if (fs.existsSync(dirPath)) {
    log(prefix, `Removing ${dirPath}...`);
    fs.rmSync(dirPath, { recursive: true, force: true });
  }
}

export function executeScript(prefix, actions) {
  const target = process.argv[2] || 'all';
  
  if (actions[target]) {
    actions[target]();
  } else if (target === 'all' && actions.all) {
    actions.all();
  } else {
    error(prefix, `Unknown target: ${target}`);
  }
}