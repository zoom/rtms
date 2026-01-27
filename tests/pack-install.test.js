/**
 * Integration test: npm pack → install → load
 *
 * This test validates the end-user installation flow by:
 * 1. Creating an npm tarball with `npm pack`
 * 2. Installing the tarball in a temporary directory
 * 3. Copying the local build or extracting prebuilds
 * 4. Verifying the native module loads correctly
 *
 * This catches issues like missing files, broken package.json config,
 * or module resolution problems that would affect users.
 *
 * Supports two modes:
 * - Local development: Uses build/Release/ from `task build:js`
 * - CI: Uses prebuilds/*.tar.gz downloaded from build artifacts
 */

const { execSync } = require('child_process');
const { mkdtempSync, rmSync, existsSync, readdirSync, writeFileSync, cpSync } = require('fs');
const { join } = require('path');
const { tmpdir } = require('os');
const tar = require('tar');

// Path to local build directory (created by `task build:js`)
const LOCAL_BUILD_DIR = join(process.cwd(), 'build', 'Release');
// Path to prebuilds directory (downloaded from CI artifacts)
const PREBUILDS_DIR = join(process.cwd(), 'prebuilds');

/**
 * Find the appropriate prebuild tarball for the current platform
 * Searches in prebuilds/ and prebuilds/@zoom/ directories
 */
function findPrebuildTarball() {
  if (!existsSync(PREBUILDS_DIR)) {
    return null;
  }

  const platform = process.platform;
  const arch = process.arch;

  // Search directories where prebuilds might be located
  const searchDirs = [
    PREBUILDS_DIR,
    join(PREBUILDS_DIR, '@zoom'),
    join(PREBUILDS_DIR, '@zoom', 'rtms')
  ];

  for (const dir of searchDirs) {
    if (!existsSync(dir)) continue;

    const files = readdirSync(dir);
    // Format: rtms-v{version}-napi-v{napi}-{platform}-{arch}.tar.gz
    const tarball = files.find(f =>
      f.endsWith('.tar.gz') &&
      f.includes(platform) &&
      f.includes(arch)
    );

    if (tarball) {
      return join(dir, tarball);
    }
  }

  return null;
}

/**
 * Get the build directory - either local build or extract from prebuild
 */
function prepareBuildDir() {
  // First, check for local build (from task build:js)
  if (existsSync(LOCAL_BUILD_DIR) && existsSync(join(LOCAL_BUILD_DIR, 'rtms.node'))) {
    return LOCAL_BUILD_DIR;
  }

  // Second, check for prebuild tarball (from CI artifacts)
  const tarball = findPrebuildTarball();
  if (tarball) {
    // Extract prebuild to project root (tarball contains build/Release/ prefix)
    // This extracts to ./build/Release/
    tar.extract({
      file: tarball,
      cwd: process.cwd(),
      sync: true
    });

    // The tarball extracts to build/Release/ at project root
    const buildDir = join(process.cwd(), 'build', 'Release');
    return buildDir;
  }

  return null;
}

/**
 * Extract macOS framework archives (.framework.tar.gz) in a directory
 * Same logic as scripts/install.js extractFrameworks()
 */
function extractFrameworks(buildDir) {
  if (process.platform !== 'darwin') {
    return;
  }

  if (!existsSync(buildDir)) {
    return;
  }

  const files = readdirSync(buildDir);
  const frameworkArchives = files.filter(f => f.endsWith('.framework.tar.gz'));

  for (const archive of frameworkArchives) {
    const archivePath = join(buildDir, archive);
    try {
      tar.extract({
        file: archivePath,
        cwd: buildDir,
        sync: true
      });
      // Remove the archive after extraction
      rmSync(archivePath);
    } catch (err) {
      console.warn(`Failed to extract ${archive}: ${err.message}`);
    }
  }
}

/**
 * Install package from tarball and copy build artifacts
 * This simulates what prebuild-install does, using local build or extracted prebuilds
 */
function installWithLocalBuild(tempDir, tarballPath, buildDir) {
  // Create minimal package.json for ESM
  writeFileSync(join(tempDir, 'package.json'), JSON.stringify({ type: 'module' }));

  // Install from tarball (ignore prebuild-install failure since we'll copy local build)
  execSync(`npm install ${tarballPath} --ignore-scripts`, {
    cwd: tempDir,
    stdio: 'pipe',
    env: { ...process.env, npm_config_loglevel: 'error' }
  });

  // Copy build artifacts to simulate prebuild-install
  const targetBuildDir = join(tempDir, 'node_modules/@zoom/rtms/build/Release');
  cpSync(buildDir, targetBuildDir, { recursive: true });

  // Extract framework archives (same as scripts/install.js does)
  extractFrameworks(targetBuildDir);
}

describe('npm pack → install → load integration', () => {
  let tempDir;
  let tarballPath;
  let buildDir;

  beforeAll(() => {
    // Prepare build directory (local build or extracted prebuild)
    buildDir = prepareBuildDir();
    if (!buildDir) {
      throw new Error(
        'No build artifacts found. Either:\n' +
        '  - Run "task build:js" for local development, or\n' +
        '  - Ensure prebuilds/*.tar.gz exists (downloaded from CI artifacts)'
      );
    }

    // Verify the build directory has the native module
    if (!existsSync(join(buildDir, 'rtms.node'))) {
      throw new Error(`Native module not found at ${buildDir}/rtms.node`);
    }

    console.log(`Using build artifacts from: ${buildDir}`);

    // Create tarball from current package
    const output = execSync('npm pack --json', { encoding: 'utf8' });
    const packages = JSON.parse(output);
    const { filename } = packages[0];
    tarballPath = join(process.cwd(), filename);
  });

  beforeEach(() => {
    // Create fresh temp directory for each test
    tempDir = mkdtempSync(join(tmpdir(), 'rtms-test-'));
  });

  afterEach(() => {
    // Clean up temp directory
    if (tempDir && existsSync(tempDir)) {
      rmSync(tempDir, { recursive: true, force: true });
    }
  });

  afterAll(() => {
    // Clean up tarball
    if (tarballPath && existsSync(tarballPath)) {
      rmSync(tarballPath);
    }
  });

  test('tarball contains required files', () => {
    const output = execSync(`tar -tzf ${tarballPath}`, { encoding: 'utf8' });
    const files = output.split('\n').filter(Boolean);

    // Core files that must be present
    expect(files).toEqual(
      expect.arrayContaining([
        'package/scripts/install.js',
        'package/index.ts',
        'package/rtms.d.ts',
        'package/package.json',
        'package/CMakeLists.txt',
        'package/src/node.cpp',
        'package/src/rtms.cpp',
        'package/src/rtms.h'
      ])
    );
  });

  test('native module loads after install from tarball', () => {
    installWithLocalBuild(tempDir, tarballPath, buildDir);

    // Verify native module loads and has expected exports (matching test.js usage patterns)
    const testScript = `
      import rtms from '@zoom/rtms';
      const checks = [
        typeof rtms.Client === 'function',
        typeof rtms.onWebhookEvent === 'function',
        typeof rtms.MEDIA_TYPE_AUDIO !== 'undefined',
        typeof rtms.MEDIA_TYPE_VIDEO !== 'undefined'
      ];
      console.log(checks.every(Boolean) ? 'OK' : 'FAIL');
    `;

    const result = execSync(`node -e "${testScript}"`, {
      cwd: tempDir,
      encoding: 'utf8'
    });

    expect(result.trim()).toBe('OK');
  });

  test('macOS frameworks are present in build', () => {
    if (process.platform !== 'darwin') {
      // Skip on non-macOS - frameworks only exist on darwin
      return;
    }

    installWithLocalBuild(tempDir, tarballPath, buildDir);

    const installedBuildDir = join(tempDir, 'node_modules/@zoom/rtms/build/Release');
    const files = readdirSync(installedBuildDir);

    // Frameworks should be present as directories (copied from local build)
    expect(files).toContain('tp.framework');
    expect(files).toContain('util.framework');
    expect(files).toContain('curl64.framework');

    // Verify frameworks are actually directories with content
    const tpFramework = join(installedBuildDir, 'tp.framework');
    expect(existsSync(tpFramework)).toBe(true);
    expect(readdirSync(tpFramework).length).toBeGreaterThan(0);
  });

  test('Client can be instantiated', () => {
    installWithLocalBuild(tempDir, tarballPath, buildDir);

    // Test that Client class works
    const testScript = `
      import rtms from '@zoom/rtms';
      const client = new rtms.Client();
      console.log(typeof client.join === 'function' ? 'OK' : 'FAIL');
    `;

    const result = execSync(`node -e "${testScript}"`, {
      cwd: tempDir,
      encoding: 'utf8'
    });

    expect(result.trim()).toBe('OK');
  });
});
