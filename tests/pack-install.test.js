/**
 * Integration test: npm pack → install → load
 *
 * This test validates the end-user installation flow by:
 * 1. Creating an npm tarball with `npm pack`
 * 2. Installing the tarball in a temporary directory
 * 3. Copying the local build (simulating prebuild-install)
 * 4. Verifying the native module loads correctly
 *
 * This catches issues like missing files, broken package.json config,
 * or module resolution problems that would affect users.
 *
 * Note: This test uses the local build directory instead of downloading
 * prebuilds from GitHub, since we need to test BEFORE publishing.
 */

const { execSync } = require('child_process');
const { mkdtempSync, rmSync, existsSync, readdirSync, writeFileSync, cpSync } = require('fs');
const { join } = require('path');
const { tmpdir } = require('os');

// Path to local build directory (created by `task build:js`)
const LOCAL_BUILD_DIR = join(process.cwd(), 'build', 'Release');

/**
 * Install package from tarball and copy local build
 * This simulates what prebuild-install does, but uses local build for testing
 */
function installWithLocalBuild(tempDir, tarballPath) {
  // Create minimal package.json for ESM
  writeFileSync(join(tempDir, 'package.json'), JSON.stringify({ type: 'module' }));

  // Install from tarball (ignore prebuild-install failure since we'll copy local build)
  execSync(`npm install ${tarballPath} --ignore-scripts`, {
    cwd: tempDir,
    stdio: 'pipe',
    env: { ...process.env, npm_config_loglevel: 'error' }
  });

  // Copy local build to simulate prebuild-install
  const targetBuildDir = join(tempDir, 'node_modules/@zoom/rtms/build/Release');
  if (!existsSync(LOCAL_BUILD_DIR)) {
    throw new Error(`Local build not found at ${LOCAL_BUILD_DIR}. Run 'task build:js' first.`);
  }
  cpSync(LOCAL_BUILD_DIR, targetBuildDir, { recursive: true });
}

describe('npm pack → install → load integration', () => {
  let tempDir;
  let tarballPath;

  beforeAll(() => {
    // Verify local build exists before running tests
    if (!existsSync(LOCAL_BUILD_DIR)) {
      throw new Error(`Local build not found at ${LOCAL_BUILD_DIR}. Run 'task build:js' first.`);
    }

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
    installWithLocalBuild(tempDir, tarballPath);

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

    installWithLocalBuild(tempDir, tarballPath);

    const buildDir = join(tempDir, 'node_modules/@zoom/rtms/build/Release');
    const files = readdirSync(buildDir);

    // Frameworks should be present as directories (copied from local build)
    expect(files).toContain('tp.framework');
    expect(files).toContain('util.framework');
    expect(files).toContain('curl64.framework');

    // Verify frameworks are actually directories with content
    const tpFramework = join(buildDir, 'tp.framework');
    expect(existsSync(tpFramework)).toBe(true);
    expect(readdirSync(tpFramework).length).toBeGreaterThan(0);
  });

  test('Client can be instantiated', () => {
    installWithLocalBuild(tempDir, tarballPath);

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
