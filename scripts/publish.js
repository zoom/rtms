#!/usr/bin/env node

/**
 * Publish script - Uploads packages to distribution channels
 *
 * Handles:
 * - Node.js prebuilds → GitHub Releases
 * - Python wheels → PyPI / TestPyPI
 * - Authentication via environment variables
 * - Platform-specific uploads
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
const PREFIX = 'Publish';

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
 * Run a command with logging
 */
function run(command, options = {}) {
  try {
    log(`Running: ${colors.dim}${command}${colors.reset}`);
    execSync(command, {
      stdio: 'inherit',
      cwd: PROJECT_ROOT,
      env: { ...process.env, ...options.env },
      ...options
    });
    return true;
  } catch (err) {
    if (options.allowFailure) {
      warning(`Command failed (continuing): ${command}`);
      return false;
    }
    error(`Command failed: ${command}\n${err.message}`);
    return false;
  }
}

/**
 * Get N-API versions from package.json
 */
function getNapiVersions() {
  const packagePath = join(PROJECT_ROOT, 'package.json');
  try {
    const packageJson = JSON.parse(fs.readFileSync(packagePath, 'utf8'));
    return packageJson.binary?.napi_versions || [10];
  } catch (err) {
    error(`Failed to read package.json: ${err.message}`);
  }
}

/**
 * Upload Node.js prebuilds to GitHub Releases
 */
function uploadNodeJS(platform = null) {
  log('Uploading Node.js prebuilds to GitHub Releases...');

  // Check for GitHub token
  if (!process.env.GITHUB_TOKEN) {
    error('GITHUB_TOKEN environment variable not set');
  }

  const napiVersions = getNapiVersions();
  const versionsStr = napiVersions.map(v => `v${v}`).join(', ');

  // Platform configurations
  const platforms = [
    { platform: 'linux', arch: 'x64' },
    { platform: 'darwin', arch: 'arm64' }
  ];

  // Filter by platform if specified
  let targetPlatforms = platforms;
  if (platform) {
    targetPlatforms = platforms.filter(p => p.platform === platform);
    if (targetPlatforms.length === 0) {
      error(`Unknown platform: ${platform}`);
    }
  }

  log(`Uploading for N-API ${versionsStr}, platforms: ${targetPlatforms.map(p => `${p.platform}-${p.arch}`).join(', ')}`);

  const prebuildCmdBase = `prebuild -r napi \
    --include-regex '\\.(node|dylib|so\\.0|tar\\.gz|ts|js|js\\.map)$' \
    --backend cmake-js`;

  const results = [];

  for (const { platform: plat, arch } of targetPlatforms) {
    for (const napiVersion of napiVersions) {
      log(`Uploading ${plat}-${arch}, N-API v${napiVersion}...`);

      // Use shell variable expansion to avoid printing token in logs
      const uploadCmd = `${prebuildCmdBase} --arch ${arch} --platform ${plat} --target ${napiVersion} -u "$GITHUB_TOKEN"`;
      const success = run(uploadCmd, { allowFailure: true });

      results.push({ platform: plat, arch, napiVersion, success });
    }
  }

  // Summary
  const successful = results.filter(r => r.success).length;
  const total = results.length;

  console.log(); // Empty line
  if (successful === total) {
    success(`All prebuilds uploaded successfully (${successful}/${total})`);
  } else {
    const failed = total - successful;
    warning(`Some uploads may have failed: ${successful}/${total} succeeded, ${failed} failed`);
  }
}

/**
 * Upload Python wheels to PyPI or TestPyPI
 */
function uploadPython(options = {}) {
  const { test = false, platform = null } = options;
  const repository = test ? 'testpypi' : 'pypi';
  const repoUrl = test
    ? 'https://test.pypi.org/legacy/'
    : 'https://upload.pypi.org/legacy/';

  log(`Uploading Python wheels to ${repository.toUpperCase()}...`);

  // Check for Twine credentials
  if (!process.env.TWINE_USERNAME || !process.env.TWINE_PASSWORD) {
    error('TWINE_USERNAME and TWINE_PASSWORD environment variables must be set');
  }

  const distDir = join(PROJECT_ROOT, 'dist', 'py');
  if (!fs.existsSync(distDir)) {
    error(`Distribution directory not found: ${distDir}\nRun: task build:py first`);
  }

  // Find wheel files
  const allWheels = fs.readdirSync(distDir).filter(f => f.endsWith('.whl'));

  if (allWheels.length === 0) {
    error(`No wheel files found in ${distDir}\nRun: task build:py first`);
  }

  // Filter by platform if specified
  let wheels = allWheels;
  if (platform) {
    wheels = allWheels.filter(w => w.includes(platform));
    if (wheels.length === 0) {
      error(`No wheels found for platform: ${platform}`);
    }
  }

  log(`Found ${wheels.length} wheel(s) to upload:`);
  wheels.forEach(wheel => {
    const wheelPath = join(distDir, wheel);
    const stats = fs.statSync(wheelPath);
    const sizeMB = (stats.size / (1024 * 1024)).toFixed(2);
    console.log(`  ${colors.dim}→${colors.reset} ${wheel} ${colors.dim}(${sizeMB} MB)${colors.reset}`);
  });

  // Upload wheels
  const wheelPaths = wheels.map(w => join(distDir, w)).join(' ');
  const uploadCmd = `twine upload --verbose --repository-url ${repoUrl} ${wheelPaths}`;

  run(uploadCmd);
  success(`Uploaded ${wheels.length} wheel(s) to ${repository.toUpperCase()}`);
}

/**
 * Parse command line arguments
 */
function parseArgs() {
  const args = process.argv.slice(2);

  const options = {
    node: args.includes('--node'),
    python: args.includes('--python'),
    test: args.includes('--test'),
    prod: args.includes('--prod'),
    platform: parsePlatformArg(args)
  };

  return options;
}

/**
 * Show usage help
 */
function showHelp() {
  console.log(`
${colors.bold}${colors.cyan}Publish Script${colors.reset} - Upload packages to distribution channels

${colors.bold}Usage:${colors.reset}
  node scripts/publish.js [options]

${colors.bold}Options:${colors.reset}
  --node                Upload Node.js prebuilds to GitHub Releases
  --python              Upload Python wheels to PyPI
  --test                Upload Python to TestPyPI (default for Python)
  --prod                Upload Python to production PyPI
  --platform=<name>     Upload only for specific platform (linux, darwin)

${colors.bold}Examples:${colors.reset}
  node scripts/publish.js --node
    ${colors.dim}→ Upload all Node.js prebuilds to GitHub${colors.reset}

  node scripts/publish.js --node --platform=linux
    ${colors.dim}→ Upload only Linux Node.js prebuilds${colors.reset}

  node scripts/publish.js --python --test
    ${colors.dim}→ Upload Python wheels to TestPyPI${colors.reset}

  node scripts/publish.js --python --prod
    ${colors.dim}→ Upload Python wheels to production PyPI${colors.reset}

${colors.bold}Environment Variables:${colors.reset}
  GITHUB_TOKEN          Required for Node.js uploads
  TWINE_USERNAME        Required for Python uploads
  TWINE_PASSWORD        Required for Python uploads

${colors.bold}Via Task:${colors.reset}
  task publish:js       Upload Node.js prebuilds
  task publish:py       Upload to production PyPI
  task publish:py:test  Upload to TestPyPI
`);
}

/**
 * Main function
 */
function main() {
  const options = parseArgs();

  // Show help if no options specified
  if (!options.node && !options.python) {
    showHelp();
    process.exit(0);
  }

  console.log(`\n${colors.bold}${colors.cyan}RTMS Package Publishing${colors.reset}\n`);

  // Upload Node.js prebuilds
  if (options.node) {
    uploadNodeJS(options.platform);
  }

  // Upload Python wheels
  if (options.python) {
    const pythonOptions = {
      test: !options.prod, // Default to test unless --prod specified
      platform: options.platform
    };
    uploadPython(pythonOptions);
  }

  console.log(); // Empty line
  success('Publishing completed successfully');
}

// Run publish
main();
