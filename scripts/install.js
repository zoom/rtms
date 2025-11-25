#!/usr/bin/env node

/**
 * Install script for npm users
 *
 * This script runs when users do `npm install @zoom/rtms`
 * It attempts to download prebuilt binaries from GitHub releases
 * If no prebuild is available, the user will need to build from source
 *
 * This script does NOT require Task - it's for end users, not contributors
 */

import { execSync } from 'child_process';
import { existsSync } from 'fs';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const colors = {
  reset: '\x1b[0m',
  cyan: '\x1b[36m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  dim: '\x1b[2m'
};

function log(message) {
  console.log(`${colors.cyan}[RTMS Install]${colors.reset} ${message}`);
}

function success(message) {
  console.log(`${colors.green}[RTMS Install]${colors.reset} ${message}`);
}

function warning(message) {
  console.log(`${colors.yellow}[RTMS Install]${colors.reset} ${message}`);
}

/**
 * Try to install prebuilt binary
 */
function installPrebuild() {
  try {
    log('Attempting to download prebuilt binary...');

    // Use prebuild-install to download from GitHub releases
    execSync('prebuild-install -r napi', {
      stdio: 'inherit',
      env: process.env
    });

    success('Prebuilt binary installed successfully!');
    return true;
  } catch (err) {
    // Prebuild not available - this is OK
    return false;
  }
}

/**
 * Main install function
 */
function main() {
  // Detect development mode (git repo exists)
  const isDevelopment = existsSync(join(__dirname, '..', '.git'));

  if (isDevelopment) {
    // In dev mode, skip prebuild - developers use `task build`
    process.exit(0);
  }

  log('Starting installation...');

  const prebuildInstalled = installPrebuild();

  if (prebuildInstalled) {
    success('Installation complete!');
    console.log();
    console.log('You can now use the RTMS SDK:');
    console.log(`  ${colors.dim}const rtms = require('@zoom/rtms');${colors.reset}`);
    console.log();
  } else {
    warning('No prebuilt binary available for your platform.');
    console.log();
    console.log('To build from source, you will need:');
    console.log(`  ${colors.dim}1. Node.js >= 20.3.0${colors.reset}`);
    console.log(`  ${colors.dim}2. CMake >= 3.25${colors.reset}`);
    console.log(`  ${colors.dim}3. C++ build tools${colors.reset}`);
    console.log(`  ${colors.dim}4. Zoom RTMS SDK files${colors.reset}`);
    console.log();
    console.log('See: https://github.com/zoom/rtms#building-from-source');
    console.log();
  }
}

main();
