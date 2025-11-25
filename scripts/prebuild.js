#!/usr/bin/env node

/**
 * Prebuild script - Creates Node.js prebuilds for distribution
 *
 * Handles:
 * - N-API version matrix (v9, v10)
 * - Platform matrix (darwin-arm64, linux-x64)
 * - Framework bundling for macOS
 * - Prebuild creation and verification
 */

import { execSync } from 'child_process';
import fs from 'fs';
import { join } from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';
import { parsePlatformArg, colors, log as logUtil, success as successUtil, error as errorUtil, warning as warningUtil } from './common.js';

// Get project root
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const PROJECT_ROOT = join(__dirname, '..');

// Wrapper functions for logging
const PREFIX = 'Prebuild';

function log(message) {
  logUtil(PREFIX, message);
}

function success(message) {
  successUtil(PREFIX, message);
}

function error(message) {
  errorUtil(PREFIX, message);
  process.exit(1);
}

function warning(message) {
  warningUtil(PREFIX, message);
}

/**
 * Get N-API versions from package.json
 */
function getNapiVersions() {
  const packagePath = join(PROJECT_ROOT, 'package.json');
  try {
    const packageJson = JSON.parse(fs.readFileSync(packagePath, 'utf8'));
    return packageJson.binary?.napi_versions || [10]; // Default to [10] if not specified
  } catch (err) {
    error(`Failed to read package.json: ${err.message}`);
  }
}

/**
 * Run a command with logging
 */
function run(command) {
  try {
    log(`Running: ${colors.dim}${command}${colors.reset}`);
    execSync(command, {
      stdio: 'inherit',
      cwd: PROJECT_ROOT,
      env: process.env
    });
    return true;
  } catch (err) {
    error(`Command failed: ${command}\n${err.message}`);
    return false;
  }
}

/**
 * Create prebuild for a specific platform and N-API version
 */
function createPrebuild(platform, arch, napiVersion) {
  const prebuildCmd = `prebuild -r napi \
    --include-regex '\\.(node|dylib|so\\.0|tar\\.gz|ts|js|js\\.map)$' \
    --backend cmake-js \
    --arch ${arch} \
    --platform ${platform} \
    --target ${napiVersion}`;

  log(`Building for ${platform}-${arch}, N-API v${napiVersion}...`);
  return run(prebuildCmd);
}

/**
 * Verify prebuild was created
 */
function verifyPrebuild(platform, arch, napiVersion) {
  const prebuildDir = join(PROJECT_ROOT, 'prebuilds', '@zoom');
  const expectedPattern = `${platform}-${arch}`;

  if (!fs.existsSync(prebuildDir)) {
    warning(`Prebuild directory not found: ${prebuildDir}`);
    return false;
  }

  const files = fs.readdirSync(prebuildDir);
  const found = files.some(file => file.includes(expectedPattern) && file.includes(`napi-v${napiVersion}`));

  if (found) {
    success(`Verified prebuild for ${platform}-${arch}, N-API v${napiVersion}`);
    return true;
  } else {
    warning(`Prebuild not found for ${platform}-${arch}, N-API v${napiVersion}`);
    return false;
  }
}

/**
 * Main prebuild function
 */
function main() {
  // Parse command line arguments
  const args = process.argv.slice(2);
  const platformArg = parsePlatformArg(args);

  // Platform configurations
  const platforms = [
    { platform: 'linux', arch: 'x64' },
    { platform: 'darwin', arch: 'arm64' }
  ];

  // Filter platforms if specified
  let targetPlatforms = platforms;
  if (platformArg) {
    targetPlatforms = platforms.filter(p => p.platform === platformArg);
    if (targetPlatforms.length === 0) {
      error(`Unknown platform: ${platformArg}. Valid platforms: linux, darwin`);
    }
  }

  // Get N-API versions
  const napiVersions = getNapiVersions();
  const versionsStr = napiVersions.map(v => `v${v}`).join(', ');

  log(`Creating Node.js prebuilds for N-API ${versionsStr}...`);
  log(`Target platforms: ${targetPlatforms.map(p => `${p.platform}-${p.arch}`).join(', ')}`);

  // Track results
  const results = [];

  // Build for each platform and N-API version
  for (const { platform, arch } of targetPlatforms) {
    for (const napiVersion of napiVersions) {
      const success = createPrebuild(platform, arch, napiVersion);
      results.push({
        platform,
        arch,
        napiVersion,
        success
      });

      if (success) {
        // Verify the prebuild was created
        verifyPrebuild(platform, arch, napiVersion);
      }
    }
  }

  // Summary
  console.log(); // Empty line
  const successful = results.filter(r => r.success).length;
  const total = results.length;

  if (successful === total) {
    success(`All prebuilds created successfully (${successful}/${total})`);
  } else {
    const failed = total - successful;
    error(`Some prebuilds failed: ${successful}/${total} succeeded, ${failed} failed`);
  }

  // List created prebuilds
  const prebuildDir = join(PROJECT_ROOT, 'prebuilds');
  if (fs.existsSync(prebuildDir)) {
    console.log(`\n${colors.cyan}Created prebuilds:${colors.reset}`);
    const files = fs.readdirSync(prebuildDir);
    files.forEach(file => {
      const filePath = join(prebuildDir, file);
      const stats = fs.statSync(filePath);
      const sizeKB = (stats.size / 1024).toFixed(2);
      console.log(`  ${colors.dim}→${colors.reset} ${file} ${colors.dim}(${sizeKB} KB)${colors.reset}`);
    });
  }

  console.log(); // Empty line
}

// Run prebuild
main();
