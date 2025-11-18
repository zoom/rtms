#!/usr/bin/env node
import os from 'os';
import fs from 'fs';
import { join } from 'path';
import { checkDeps } from './check-deps.js';
import { setupFrameworks } from './frameworks.js';
import { log, run, executeScript, getBuildMode, getProjectRoot } from './utils.js';

const PREFIX = "Build";

await checkDeps();

// Platform detection utilities
function getCurrentPlatform() {
  return `${os.platform()}-${os.arch()}`;
}

function needsDocker(targetPlatform) {
  const current = getCurrentPlatform();
  // Use Docker when building for linux-x64 from darwin-arm64
  return targetPlatform === 'linux' && current === 'darwin-arm64';
}

// Node.js build functions
function buildNodeJS(platform = null) {
  const buildMode = getBuildMode();
  const debugFlag = buildMode === 'debug' ? ' --debug' : '';

  if (platform) {
    log(PREFIX, `Building Node.js module for ${platform}...`);
    // Platform-specific build would use Docker or cross-compilation
    // For now, build for current platform
    run(`cmake-js compile${debugFlag}`, PREFIX);
  } else {
    log(PREFIX, 'Building Node.js module for current platform...');
    run(`cmake-js compile${debugFlag}`, PREFIX);
  }

  log(PREFIX, `Node.js module built in ${buildMode} mode`);
}

function buildNodeJSLinux() {
  if (needsDocker('linux')) {
    log(PREFIX, 'Building Node.js module for Linux via Docker...');
    run(`docker compose up js`, PREFIX);
  } else {
    log(PREFIX, 'Building Node.js module for Linux...');
    buildNodeJS('linux');
  }
}

function buildNodeJSDarwin() {
  log(PREFIX, 'Building Node.js module for macOS...');
  buildNodeJS('darwin');
}

// Python build functions
function buildPython(platform = null) {
  const buildMode = getBuildMode();
  const cmakeBuildType = buildMode === 'debug' ? 'Debug' : 'Release';

  if (platform) {
    log(PREFIX, `Building Python wheel for ${platform}...`);
    // Platform-specific build would use Docker or cross-compilation
    run(`python3 -m build --wheel -Ccmake.build-type='${cmakeBuildType}' --outdir dist/py`, PREFIX);
  } else {
    log(PREFIX, 'Building Python wheel for current platform...');
    run(`python3 -m build --wheel -Ccmake.build-type='${cmakeBuildType}' --outdir dist/py`, PREFIX);
  }

  log(PREFIX, `Python wheel built in ${buildMode} mode`);
}

function buildPythonLinux() {
  if (needsDocker('linux')) {
    log(PREFIX, 'Building Python wheel for Linux via Docker...');
    run(`docker compose up py`, PREFIX);
  } else {
    log(PREFIX, 'Building Python wheel for Linux...');
    buildPython('linux');
  }
}

function buildPythonDarwin() {
  log(PREFIX, 'Building Python wheel for macOS...');
  buildPython('darwin');
}

// Go build functions
function buildGo(platform = null) {
  const buildMode = getBuildMode();
  const debugFlag = buildMode === 'debug' ? ' --debug' : '';

  if (platform) {
    log(PREFIX, `Building Go module for ${platform}...`);
  } else {
    log(PREFIX, 'Building Go module for current platform...');
  }

  run(`cmake-js configure${debugFlag} --CDGO=true`, PREFIX);
  log(PREFIX, `Go module built in ${buildMode} mode`);
}

function buildGoLinux() {
  log(PREFIX, 'Building Go module for Linux...');
  buildGo('linux');
}

function buildGoDarwin() {
  log(PREFIX, 'Building Go module for macOS...');
  buildGo('darwin');
}

// Node.js prebuild functions
const prebuildCmdBase = "prebuild -r napi \
    --include-regex '\.(node|dylib|so.0|tar.gz|ts|js|js.map)$'  \
    --backend cmake-js";

// Read N-API versions from package.json
function getNapiVersions() {
  const packagePath = join(getProjectRoot(), 'package.json');
  const packageJson = JSON.parse(fs.readFileSync(packagePath, 'utf8'));
  return packageJson.binary?.napi_versions || [10]; // Default to [10] if not specified
}

const NAPI_VERSIONS = getNapiVersions();

function prebuildNodeJS(platform = null) {
  const versions = NAPI_VERSIONS.map(v => `v${v}`).join(' and ');

  if (platform === 'linux') {
    log(PREFIX, `Generating Node.js prebuilds for Linux (x64) with N-API ${versions}...`);
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Building for N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch x64 --platform linux --target ${napiVersion}`, PREFIX);
    }
  } else if (platform === 'darwin') {
    log(PREFIX, `Generating Node.js prebuilds for macOS (arm64) with N-API ${versions}...`);
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Building for N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch arm64 --platform darwin --target ${napiVersion}`, PREFIX);
    }
  } else {
    log(PREFIX, `Generating Node.js prebuilds for all platforms with N-API ${versions}...`);
    // Build for Linux x64
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Building for Linux x64, N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch x64 --platform linux --target ${napiVersion}`, PREFIX);
    }
    // Build for Darwin arm64
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Building for macOS arm64, N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch arm64 --platform darwin --target ${napiVersion}`, PREFIX);
    }
  }
}

function prebuildNodeJSLinux() {
  prebuildNodeJS('linux');
}

function prebuildNodeJSDarwin() {
  prebuildNodeJS('darwin');
}

// Python prebuild functions (wheels are the prebuilds)
function prebuildPython(platform = null) {
  log(PREFIX, 'Python prebuilds are the same as builds (wheels)');
  buildPython(platform);
}

function prebuildPythonLinux() {
  prebuildPython('linux');
}

function prebuildPythonDarwin() {
  prebuildPython('darwin');
}

// Upload functions for Node.js (GitHub releases)
function uploadNodeJS(platform = null) {
  const versions = NAPI_VERSIONS.map(v => `v${v}`).join(' and ');

  if (platform === 'linux') {
    log(PREFIX, `Uploading Node.js prebuilds for Linux (x64) with N-API ${versions} to GitHub...`);
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Uploading N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch x64 --platform linux --target ${napiVersion} -u "$GITHUB_TOKEN"`, PREFIX);
    }
  } else if (platform === 'darwin') {
    log(PREFIX, `Uploading Node.js prebuilds for macOS (arm64) with N-API ${versions} to GitHub...`);
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Uploading N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch arm64 --platform darwin --target ${napiVersion} -u "$GITHUB_TOKEN"`, PREFIX);
    }
  } else {
    log(PREFIX, `Uploading Node.js prebuilds for all platforms with N-API ${versions} to GitHub...`);
    // Upload for Linux x64
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Uploading for Linux x64, N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch x64 --platform linux --target ${napiVersion} -u "$GITHUB_TOKEN"`, PREFIX);
    }
    // Upload for Darwin arm64
    for (const napiVersion of NAPI_VERSIONS) {
      log(PREFIX, `Uploading for macOS arm64, N-API v${napiVersion}...`);
      run(`${prebuildCmdBase} --arch arm64 --platform darwin --target ${napiVersion} -u "$GITHUB_TOKEN"`, PREFIX);
    }
  }
}

function uploadNodeJSLinux() {
  uploadNodeJS('linux');
}

function uploadNodeJSDarwin() {
  uploadNodeJS('darwin');
}

// Upload functions for Python (PyPI/TestPyPI)
function uploadPython(target = 'test', platform = null) {
  const repository = target === 'prod' ? 'pypi' : 'testpypi';
  const repoUrl = target === 'prod'
    ? 'https://upload.pypi.org/legacy/'
    : 'https://test.pypi.org/legacy/';

  if (platform) {
    log(PREFIX, `Uploading Python wheel for ${platform} to ${repository}...`);
    run(`twine upload --repository-url ${repoUrl} dist/py/*${platform}*.whl`, PREFIX);
  } else {
    log(PREFIX, `Uploading all Python wheels to ${repository}...`);
    run(`twine upload --repository-url ${repoUrl} dist/py/*.whl`, PREFIX);
  }
}

function uploadPythonLinux() {
  uploadPython('test', 'linux');
}

function uploadPythonDarwin() {
  uploadPython('test', 'darwin');
}

function uploadPythonProdLinux() {
  uploadPython('prod', 'linux');
}

function uploadPythonProdDarwin() {
  uploadPython('prod', 'darwin');
}

function uploadPythonAll() {
  uploadPython('test', null);
}

function uploadPythonProdAll() {
  uploadPython('prod', null);
}

// Legacy aliases for backward compatibility (to be removed)
function buildAll() {
  buildNodeJS();
  buildPython();
  buildGo();
}

// Run using our common execution wrapper
executeScript(PREFIX, {
  // Node.js builds
  'js': buildNodeJS,
  'js:linux': buildNodeJSLinux,
  'js:darwin': buildNodeJSDarwin,

  // Python builds
  'python': buildPython,
  'py': buildPython,
  'py:linux': buildPythonLinux,
  'py:darwin': buildPythonDarwin,

  // Go builds
  'go': buildGo,
  'go:linux': buildGoLinux,
  'go:darwin': buildGoDarwin,

  // Node.js prebuilds
  'prebuild:js': prebuildNodeJS,
  'prebuild:js:linux': prebuildNodeJSLinux,
  'prebuild:js:darwin': prebuildNodeJSDarwin,

  // Python prebuilds
  'prebuild:py': prebuildPython,
  'prebuild:py:linux': prebuildPythonLinux,
  'prebuild:py:darwin': prebuildPythonDarwin,

  // Node.js uploads
  'upload:js': uploadNodeJS,
  'upload:js:linux': uploadNodeJSLinux,
  'upload:js:darwin': uploadNodeJSDarwin,

  // Python uploads (TestPyPI)
  'upload:py': uploadPythonAll,
  'upload:py:linux': uploadPythonLinux,
  'upload:py:darwin': uploadPythonDarwin,

  // Python uploads (Production PyPI)
  'upload:py:prod': uploadPythonProdAll,
  'upload:py:prod:linux': uploadPythonProdLinux,
  'upload:py:prod:darwin': uploadPythonProdDarwin,

  // Legacy commands
  'setupFrameworks': setupFrameworks,
  'all': buildAll,

  // Legacy upload commands (deprecated)
  'upload': uploadNodeJS,
  'upload_linux': uploadNodeJSLinux,
  'prebuild': prebuildNodeJS,
});