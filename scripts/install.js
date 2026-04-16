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
import { existsSync, readdirSync, unlinkSync } from 'fs';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';
import * as tar from 'tar';

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
 * Extract macOS framework archives (.framework.tar.gz) after prebuild-install
 *
 * The prebuilt binaries include macOS frameworks as tar.gz archives to reduce size.
 * These must be extracted for the native module to load correctly.
 */
function extractFrameworks(buildDir) {
  if (process.platform !== 'darwin') {
    // Frameworks only exist on macOS
    return;
  }

  if (!existsSync(buildDir)) {
    return;
  }

  const files = readdirSync(buildDir);
  const frameworkArchives = files.filter(f => f.endsWith('.framework.tar.gz'));

  if (frameworkArchives.length === 0) {
    return;
  }

  log(`Extracting ${frameworkArchives.length} framework archive(s)...`);

  for (const archive of frameworkArchives) {
    const archivePath = join(buildDir, archive);
    try {
      tar.extract({
        file: archivePath,
        cwd: buildDir,
        sync: true
      });
      // Remove the archive after extraction to save space
      unlinkSync(archivePath);
    } catch (err) {
      warning(`Failed to extract ${archive}: ${err.message}`);
    }
  }
}

/**
 * Try to install prebuilt binary
 */
function installPrebuild() {
  try {
    log('Attempting to download prebuilt binary...');

    // Use prebuild-install to download from GitHub releases.
    // Resolve from node_modules/.bin/ so this works after `npm install`
    // where the binary is local, not in system PATH.
    const prebuildInstallBin = join(__dirname, '..', 'node_modules', '.bin', 'prebuild-install');
    execSync(`${prebuildInstallBin} -r napi`, {
      stdio: 'inherit',
      env: process.env
    });

    // Extract framework archives on macOS
    const buildDir = join(__dirname, '..', 'build', 'Release');
    extractFrameworks(buildDir);

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
  // Detect development mode (git repo exists).
  // Pass --force to bypass this check (e.g. CI extracting a downloaded prebuild).
  const isDevelopment = existsSync(join(__dirname, '..', '.git'));
  const isForced = process.argv.includes('--force');

  if (isDevelopment && !isForced) {
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
