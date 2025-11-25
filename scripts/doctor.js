#!/usr/bin/env node

/**
 * Doctor script - Verifies system meets minimum requirements
 *
 * Checks:
 * - Node.js version (>=20.3.0)
 * - Python version (>=3.10)
 * - CMake version (>=3.25)
 * - Docker version (>=20.0)
 * - Zoom RTMS SDK presence
 */

import { execSync } from 'child_process';
import fs from 'fs';
import os from 'os';
import { join } from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';

// Get project root
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const PROJECT_ROOT = join(__dirname, '..');

// Color codes
const colors = {
  reset: '\x1b[0m',
  red: '\x1b[31m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  cyan: '\x1b[36m',
  dim: '\x1b[2m',
  bold: '\x1b[1m'
};

// Requirements configuration
const REQUIREMENTS = {
  node: {
    min: '20.3.0',
    recommended: '24',
    command: 'node --version',
    parser: (output) => output.trim().replace('v', '')
  },
  python: {
    min: '3.10',
    recommended: '3.13',
    command: 'python3 --version',
    parser: (output) => output.trim().split(' ')[1]
  },
  cmake: {
    min: '3.25',
    recommended: '3.28',
    command: 'cmake --version',
    parser: (output) => output.trim().split('\n')[0].split(' ')[2]
  },
  docker: {
    min: '20.0',
    recommended: '28.5',
    command: 'docker --version',
    parser: (output) => output.trim().split(' ')[2].replace(',', ''),
    optional: true  // Docker is optional (only needed for Linux builds)
  },
  task: {
    min: '3.0',
    recommended: '3.x',
    command: 'task --version',
    parser: (output) => output.trim().split(' ').pop(),
    optional: true  // Task is what we're introducing, so it may not be installed yet
  }
};

/**
 * Parse semantic version string to comparable format
 */
function parseVersion(versionStr) {
  const parts = versionStr.split(/[.-]/).map(p => parseInt(p, 10) || 0);
  return {
    major: parts[0] || 0,
    minor: parts[1] || 0,
    patch: parts[2] || 0,
    raw: versionStr
  };
}

/**
 * Compare two semantic versions
 * Returns: -1 if v1 < v2, 0 if equal, 1 if v1 > v2
 */
function compareVersions(v1, v2) {
  const ver1 = parseVersion(v1);
  const ver2 = parseVersion(v2);

  if (ver1.major !== ver2.major) return ver1.major - ver2.major;
  if (ver1.minor !== ver2.minor) return ver1.minor - ver2.minor;
  return ver1.patch - ver2.patch;
}

/**
 * Get version of a tool
 */
function getVersion(command) {
  try {
    const output = execSync(command, { encoding: 'utf-8', stderr: 'pipe' });
    return output;
  } catch {
    return null;
  }
}

/**
 * Check if a requirement is met
 */
function checkRequirement(name, config) {
  const version = getVersion(config.command);

  if (!version) {
    if (config.optional) {
      console.log(`${colors.yellow}⚠${colors.reset} ${colors.bold}${name}${colors.reset} not found ${colors.dim}(optional)${colors.reset}`);
      return { status: 'optional', name };
    } else {
      console.log(`${colors.red}✗${colors.reset} ${colors.bold}${name}${colors.reset} not found`);
      console.log(`${colors.dim}  Install: ${getInstallInstructions(name)}${colors.reset}`);
      return { status: 'missing', name };
    }
  }

  const parsedVersion = config.parser(version);
  const comparison = compareVersions(parsedVersion, config.min);

  if (comparison < 0) {
    console.log(`${colors.red}✗${colors.reset} ${colors.bold}${name}${colors.reset} ${parsedVersion} ${colors.dim}(need ${config.min}+)${colors.reset}`);
    console.log(`${colors.dim}  Upgrade: ${getUpgradeInstructions(name)}${colors.reset}`);
    return { status: 'outdated', name, version: parsedVersion, required: config.min };
  }

  // Check if it's the recommended version
  if (parsedVersion.startsWith(config.recommended.replace('.x', '').replace('+', ''))) {
    console.log(`${colors.green}✓${colors.reset} ${colors.bold}${name}${colors.reset} ${parsedVersion} ${colors.dim}(recommended)${colors.reset}`);
    return { status: 'recommended', name, version: parsedVersion };
  } else {
    console.log(`${colors.yellow}⚠${colors.reset} ${colors.bold}${name}${colors.reset} ${parsedVersion} ${colors.dim}(works, but ${config.recommended} recommended)${colors.reset}`);
    return { status: 'works', name, version: parsedVersion };
  }
}

/**
 * Get install instructions for a tool
 */
function getInstallInstructions(tool) {
  const instructions = {
    node: 'https://nodejs.org/ or use nvm/fnm',
    python: 'https://www.python.org/downloads/',
    cmake: 'brew install cmake (macOS) or https://cmake.org/download/',
    docker: 'https://docs.docker.com/get-docker/',
    task: 'brew install go-task (macOS) or https://taskfile.dev/installation/'
  };
  return instructions[tool] || 'See project README';
}

/**
 * Get upgrade instructions for a tool
 */
function getUpgradeInstructions(tool) {
  const instructions = {
    node: 'nvm install 24 or download from https://nodejs.org/',
    python: 'pyenv install 3.13 or download from https://www.python.org/',
    cmake: 'brew upgrade cmake or download from https://cmake.org/',
    docker: 'Check Docker Desktop for updates',
    task: 'brew upgrade go-task or download from https://taskfile.dev/'
  };
  return instructions[tool] || 'Check official documentation';
}

/**
 * Check if we're running inside a Docker container
 */
function isInsideDocker() {
  try {
    return fs.existsSync('/.dockerenv');
  } catch {
    return false;
  }
}

/**
 * Check for Zoom RTMS SDK
 */
function checkSDK() {
  const platform = `${os.platform()}-${os.arch()}`;

  // Only darwin-arm64 and linux-x64 are supported
  const platformMap = {
    'darwin-arm64': 'darwin-arm64',
    'linux-x64': 'linux-x64'
  };

  const mappedPlatform = platformMap[platform];

  if (!mappedPlatform) {
    console.log(`${colors.red}✗${colors.reset} ${colors.bold}Platform not supported${colors.reset} ${colors.dim}(${platform})${colors.reset}`);
    console.log(`${colors.dim}  Supported platforms: darwin-arm64, linux-x64${colors.reset}`);
    return { status: 'unsupported', name: 'Platform' };
  }

  const sdkDir = join(PROJECT_ROOT, 'lib', mappedPlatform);

  // Check for library files
  const libraryFiles = {
    'darwin-arm64': ['librtms_sdk.dylib', 'librtmsdk.dylib'], // Check both naming conventions
    'linux-x64': ['librtmsdk.so.0']
  };

  const expectedFiles = libraryFiles[mappedPlatform] || [];
  const foundFiles = expectedFiles.filter(file => {
    const path = join(sdkDir, file);
    return fs.existsSync(path);
  });

  if (foundFiles.length === 0) {
    console.log(`${colors.red}✗${colors.reset} ${colors.bold}Zoom RTMS SDK${colors.reset} not found in ${colors.dim}lib/${mappedPlatform}/${colors.reset}`);
    console.log(`${colors.dim}  Run: task setup (or node scripts/check-deps.js)${colors.reset}`);
    return { status: 'missing', name: 'SDK' };
  }

  // Check for header files
  const includeDir = join(PROJECT_ROOT, 'lib', 'include');
  const requiredHeaders = ['rtms_csdk.h', 'rtms_common.h'];
  const missingHeaders = requiredHeaders.filter(header => {
    return !fs.existsSync(join(includeDir, header));
  });

  if (missingHeaders.length > 0) {
    console.log(`${colors.red}✗${colors.reset} ${colors.bold}RTMS SDK headers${colors.reset} incomplete ${colors.dim}(missing: ${missingHeaders.join(', ')})${colors.reset}`);
    console.log(`${colors.dim}  Run: task setup (or node scripts/check-deps.js)${colors.reset}`);
    return { status: 'incomplete', name: 'SDK Headers' };
  }

  console.log(`${colors.green}✓${colors.reset} ${colors.bold}Zoom RTMS SDK${colors.reset} found ${colors.dim}(${mappedPlatform})${colors.reset}`);
  return { status: 'ok', name: 'SDK' };
}

/**
 * Main doctor function
 */
function main() {
  console.log(`\n${colors.bold}${colors.cyan}RTMS SDK System Requirements Check${colors.reset}\n`);

  const results = [];
  const insideDocker = isInsideDocker();

  if (insideDocker) {
    console.log(`${colors.cyan}ℹ${colors.reset} ${colors.dim}Running inside Docker container${colors.reset}\n`);
  }

  // Check all requirements
  for (const [name, config] of Object.entries(REQUIREMENTS)) {
    // Skip docker check if we're inside Docker
    if (name === 'docker' && insideDocker) {
      console.log(`${colors.dim}⊘ ${colors.bold}docker${colors.reset} ${colors.dim}(skipped - running inside container)${colors.reset}`);
      continue;
    }
    results.push(checkRequirement(name, config));
  }

  // Check SDK
  results.push(checkSDK());

  // Summary
  console.log(); // Empty line

  const failed = results.filter(r => r.status === 'missing' || r.status === 'outdated');
  const warnings = results.filter(r => r.status === 'works' || r.status === 'optional' || r.status === 'incomplete');

  if (failed.length > 0) {
    console.log(`${colors.red}${colors.bold}✗ System does not meet requirements${colors.reset}`);
    console.log(`${colors.dim}  ${failed.length} required tool(s) missing or outdated${colors.reset}\n`);
    process.exit(1);
  } else if (warnings.length > 0) {
    console.log(`${colors.yellow}${colors.bold}⚠ System meets minimum requirements${colors.reset}`);
    console.log(`${colors.dim}  Consider upgrading for the best experience${colors.reset}\n`);
  } else {
    console.log(`${colors.green}${colors.bold}✓ All requirements met!${colors.reset}`);
    console.log(`${colors.dim}  Your system is ready for development${colors.reset}\n`);
  }
}

// Run doctor
main();
