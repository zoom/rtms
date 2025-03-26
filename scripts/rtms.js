#!/usr/bin/env node

import fs from 'fs';
import { join } from 'path';
import { execSync } from 'child_process';
import { log, error, success, getProjectRoot } from './common/utils.js';
import { setupFrameworks } from './common/frameworks.js';

const PREFIX = "CLI";
const CONFIG = join(getProjectRoot(), 'scripts', '.rtms-config.json');

function printHelp() {
    console.log(`
  \x1b[1mRTMS Command Line Interface\x1b[0m
  
  Usage: 
    npm run rtms [command] [target] [options]
  
  Commands:
    build   - Build the project (js, python, go, prebuild, upload, all)
    test    - Run tests (js, js:manual, python, go, all)
    docs    - Generate documentation (js, python, all)
    clean   - Clean build artifacts
    mode    - Set build mode (debug, release)
    help    - Show this help message
  
  Build Modes:
    debug   - Build with debug symbols and minimal optimization
    release - Build with optimizations and no debug symbols (default)
  
  Environment Variables:
    RTMS_BUILD_MODE - Set to 'debug' or 'release' to control build type
    NODE_ENV        - Will also be used if RTMS_BUILD_MODE is not set
  
  Examples:
    npm run rtms build js                # Build the JavaScript module (release mode)
    npm run rtms build js -- --debug     # Build the JavaScript module in debug mode
    RTMS_BUILD_MODE=debug npm run build  # Build everything in debug mode
    npm run rtms mode debug              # Set build mode to debug for future commands
    npm run rtms mode release            # Set build mode to release for future commands
    npm run rtms test python             # Test the Python module
    npm run rtms docs all                # Generate all documentation
    npm run rtms clean                   # Clean build artifacts
    `);
  }



function runCommand(scriptName, target = '') {
  const scriptPath = join(getProjectRoot(), 'scripts', 'common', `${scriptName}.js`);
  
  try {
    log(PREFIX, `Running ${scriptName} script${target ? ' for ' + target : ''}...`);
    execSync(`node ${scriptPath} ${target}`, { 
      stdio: 'inherit',
      cwd: getProjectRoot()
    });
    success(PREFIX, `${scriptName} completed successfully`);
    return true;
  } catch (err) {
    error(PREFIX, `${scriptName} failed: ${err.message}`);
    return false;
  }
}

// Set or view the current build mode
function setBuildMode(mode) {
    const CONFIG = join(getProjectRoot(), 'scripts','.rtms-config.json');
    
    // Parse command line arguments for debug/release flags
    const args = process.argv.slice(3);
    if (args.includes('--debug')) {
      mode = 'debug';
    } else if (args.includes('--release')) {
      mode = 'release';
    }
    
    // If mode is provided, update the configuration file
    if (mode) {
      if (mode !== 'debug' && mode !== 'release') {
        error(PREFIX, `Invalid build mode: ${mode}. Must be 'debug' or 'release'`);
        return false;
      }
      
      let config = {};
      if (fs.existsSync(CONFIG)) {
        try {
          const data = fs.readFileSync(CONFIG, 'utf8');
          config = JSON.parse(data);
        } catch (err) {
          warning(PREFIX, `Failed to parse config file, creating new one: ${err.message}`);
        }
      }
      
      config.buildMode = mode;
      
      try {
        fs.writeFileSync(CONFIG, JSON.stringify(config, null, 2), 'utf8');
        success(PREFIX, `Build mode set to '${mode}'`);
        
        // Also set the environment variable for this process
        process.env.RTMS_BUILD_MODE = mode;
        return true;
      } catch (err) {
        error(PREFIX, `Failed to save config: ${err.message}`);
        return false;
      }
    } else {
      // If no mode provided, show the current mode
      let currentMode = 'release'; // Default
      
      if (process.env.RTMS_BUILD_MODE) {
        currentMode = process.env.RTMS_BUILD_MODE;
      } else if (process.env.NODE_ENV === 'development' || process.env.NODE_ENV === 'debug') {
        currentMode = 'debug';
      } else if (fs.existsSync(CONFIG)) {
        try {
          const data = fs.readFileSync(CONFIG, 'utf8');
          const config = JSON.parse(data);
          if (config.buildMode) {
            currentMode = config.buildMode;
          }
        } catch (err) {
          warning(PREFIX, `Failed to read config: ${err.message}`);
        }
      }
      
      log(PREFIX, `Current build mode: ${currentMode}`);
      return true;
    }
  }
  
  function main() {
    const command = process.argv[2];
    const target = process.argv[3] || '';
    
    // Check for debug/release flags that would apply to any command
    const args = process.argv.slice(3);
    if (args.includes('--debug')) {
      process.env.RTMS_BUILD_MODE = 'debug';
    } else if (args.includes('--release')) {
      process.env.RTMS_BUILD_MODE = 'release';
    } else {
      // Load build mode from config if available
      try {
        if (fs.existsSync(CONFIG)) {
          const data = fs.readFileSync(CONFIG, 'utf8');
          const config = JSON.parse(data);
          if (config.buildMode && !process.env.RTMS_BUILD_MODE) {
            process.env.RTMS_BUILD_MODE = config.buildMode;
          }
        }
      } catch (err) {
        // Ignore errors reading config
      }
    }
    
    if (!command || command === 'help') {
      printHelp();
      return;
    }
    
      // Check for Node.js dependencies
    if (!fs.existsSync(join(getProjectRoot(), 'node_modules'))) {
      log(PREFIX, 'node_modules not found, installing dependencies...');
      try {
        execSync('npm install', { 
          stdio: 'inherit', 
          cwd: getProjectRoot() 
        });

        if (process.platform === 'darwin') {
          setupFrameworks();
        }

        success(PREFIX, 'Dependencies installed successfully');
      } catch (err) {
        error(PREFIX, `Failed to install dependencies: ${err.message}`);
        return false;
      }
    }

    switch (command) {
      case 'build':
      case 'test':
      case 'docs':
      case 'clean':
        runCommand(command, target);
        break;
      case 'mode':
        setBuildMode(target);
        break;
      default:
        error(PREFIX, `Unknown command: ${command}`);
        printHelp();
    }
  }

main();